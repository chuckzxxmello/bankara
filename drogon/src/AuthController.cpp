#include "AuthController.h"
#include "Totp.h"
#include <drogon/drogon.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>
#include <jwt-cpp/jwt.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::string bytesToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return ss.str();
}

static std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned char byte = (unsigned char)strtol(hex.substr(i, 2).c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// Legacy SHA-256 (kept for migration of old passwords)
std::string AuthController::hashPassword(const std::string& password) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Final(hash, &sha256);
    return bytesToHex(hash, SHA256_DIGEST_LENGTH);
}

// PBKDF2-HMAC-SHA256 with random 16-byte salt, 100000 iterations
std::string AuthController::hashPasswordPBKDF2(const std::string& password) const {
    unsigned char salt[16];
    RAND_bytes(salt, 16);

    unsigned char derived[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                       salt, 16, 100000,
                       EVP_sha256(), 32, derived);

    return bytesToHex(salt, 16) + ":" + bytesToHex(derived, 32);
}

bool AuthController::verifyPasswordPBKDF2(const std::string& password, const std::string& stored) const {
    auto colonPos = stored.find(':');
    if (colonPos == std::string::npos) {
        // Not PBKDF2 format — this is a legacy SHA-256 hash
        return false;
    }

    std::string saltHex = stored.substr(0, colonPos);
    std::string hashHex = stored.substr(colonPos + 1);

    auto saltBytes = hexToBytes(saltHex);
    auto storedHashBytes = hexToBytes(hashHex);

    unsigned char derived[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                       saltBytes.data(), saltBytes.size(), 100000,
                       EVP_sha256(), 32, derived);

    // Constant-time comparison to prevent timing attacks
    if (storedHashBytes.size() != 32) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < 32; i++) {
        result |= derived[i] ^ storedHashBytes[i];
    }
    return result == 0;
}

std::string AuthController::getJwtSecret() const {
    const char* env = std::getenv("JWT_SECRET");
    if (env && strlen(env) > 0) {
        return std::string(env);
    }
    std::cerr << "WARNING: JWT_SECRET environment variable not set! Using insecure default." << std::endl;
    return "super_secret_bankara_key_123";
}

std::string AuthController::generateJWT(const std::string& userId, bool mfa_pending) const {
    std::string secret = getJwtSecret();
    auto token = jwt::create()
        .set_issuer(mfa_pending ? "bankara_mfa_pending" : "bankara_auth")
        .set_type("JWS")
        .set_payload_claim("user_id", jwt::claim(std::string(userId)))
        .set_expires_at(std::chrono::system_clock::now() + (mfa_pending ? std::chrono::minutes(5) : std::chrono::hours(24)))
        .sign(jwt::algorithm::hs256{secret});
    return token;
}

void AuthController::setAuthCookie(const HttpResponsePtr& resp, const std::string& token) const {
    // HttpOnly prevents JS access (XSS protection)
    // SameSite=Strict prevents CSRF
    // Path=/api ensures cookie is only sent for API requests
    std::string cookie = "bankara_token=" + token + "; HttpOnly; SameSite=Strict; Path=/; Max-Age=86400";
    resp->addHeader("Set-Cookie", cookie);
}

void AuthController::clearAuthCookie(const HttpResponsePtr& resp) const {
    std::string cookie = "bankara_token=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0";
    resp->addHeader("Set-Cookie", cookie);
}

static bool validatePasswordStrength(const std::string& password, std::string& errorMsg) {
    if (password.length() < 8) {
        errorMsg = "Password must be at least 8 characters long";
        return false;
    }
    bool hasUpper = false, hasSpecial = false;
    for (char c : password) {
        if (isupper(c)) hasUpper = true;
        if (ispunct(c) || isspace(c)) hasSpecial = true;
    }
    if (!hasUpper || !hasSpecial) {
        errorMsg = "Password must contain at least one uppercase letter and one special character";
        return false;
    }
    return true;
}

// ─── Register ────────────────────────────────────────────────────────────────

void AuthController::registerUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("email") || !jsonPtr->isMember("password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email or password");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string email = (*jsonPtr)["email"].asString();
    std::string password = (*jsonPtr)["password"].asString();

    std::string pwError;
    if (!validatePasswordStrength(password, pwError)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(pwError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string hashed_pw = hashPasswordPBKDF2(password);
    std::string verification_token = drogon::utils::getUuid();

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "INSERT INTO users (email, password_hash, verification_token) VALUES ($1, $2, $3) RETURNING id",
        [callback, verification_token, email](const drogon::orm::Result &result) {
            std::cout << "Verification link for " << email << ": http://localhost:5150/verify-email?token=" << verification_token << std::endl;
            
            Json::Value ret;
            ret["status"] = "success";
            ret["user_id"] = result[0]["id"].as<std::string>();
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error or email already exists.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        email, hashed_pw, verification_token
    );
}

// ─── Login ───────────────────────────────────────────────────────────────────

void AuthController::loginUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("email") || !jsonPtr->isMember("password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email or password");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string email = (*jsonPtr)["email"].asString();
    std::string password = (*jsonPtr)["password"].asString();

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT id, password_hash, mfa_enabled, email_verified, failed_login_attempts, "
        "extract(epoch from lockout_until) as lockout_until_epoch, "
        "extract(epoch from now()) as now_epoch FROM users WHERE email = $1",
        [callback, this, password, dbClient, email](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid credentials");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            int failed_attempts = result[0]["failed_login_attempts"].as<int>();
            double lockout_until_epoch = result[0]["lockout_until_epoch"].isNull() ? 0 : result[0]["lockout_until_epoch"].as<double>();
            double now_epoch = result[0]["now_epoch"].as<double>();

            if (lockout_until_epoch > now_epoch) {
                int cooldown_remaining = (int)(lockout_until_epoch - now_epoch);
                Json::Value ret;
                ret["status"] = "locked";
                ret["cooldown_remaining"] = cooldown_remaining;
                ret["message"] = "Account locked due to multiple failed attempts.";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k429TooManyRequests);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string user_id = result[0]["id"].as<std::string>();
            std::string stored_hash = result[0]["password_hash"].as<std::string>();

            if (stored_hash == "DELETED") {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid credentials");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            // Try PBKDF2 first, then legacy SHA-256 with auto-migration
            bool passwordValid = false;
            bool needsMigration = false;

            if (stored_hash.find(':') != std::string::npos) {
                // PBKDF2 format
                passwordValid = verifyPasswordPBKDF2(password, stored_hash);
            } else {
                // Legacy SHA-256 format
                std::string legacyHash = hashPassword(password);
                if (legacyHash == stored_hash) {
                    passwordValid = true;
                    needsMigration = true;
                }
            }

            if (!passwordValid) {
                failed_attempts++;
                int cooldown_minutes = 0;
                if (failed_attempts >= 7) cooldown_minutes = 10;
                else if (failed_attempts >= 5) cooldown_minutes = 5;
                else if (failed_attempts >= 3) cooldown_minutes = 1;

                if (cooldown_minutes > 0) {
                    dbClient->execSqlAsync(
                        "UPDATE users SET failed_login_attempts = $1, lockout_until = now() + make_interval(mins => $2) WHERE id = $3",
                        [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                        failed_attempts, cooldown_minutes, user_id);
                    
                    Json::Value ret;
                    ret["status"] = "locked";
                    ret["cooldown_remaining"] = cooldown_minutes * 60;
                    ret["message"] = "Account locked due to multiple failed attempts.";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k429TooManyRequests);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                } else {
                    dbClient->execSqlAsync(
                        "UPDATE users SET failed_login_attempts = $1 WHERE id = $2",
                        [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                        failed_attempts, user_id);
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k401Unauthorized);
                    resp->setBody("Invalid credentials");
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                }
                return;
            }

            // Successful login — clear failed attempts
            if (failed_attempts > 0 || lockout_until_epoch > 0) {
                dbClient->execSqlAsync(
                    "UPDATE users SET failed_login_attempts = 0, lockout_until = NULL WHERE id = $1",
                    [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                    user_id);
            }

            // Auto-migrate legacy SHA-256 → PBKDF2
            if (needsMigration) {
                std::string newHash = hashPasswordPBKDF2(password);
                dbClient->execSqlAsync(
                    "UPDATE users SET password_hash = $1 WHERE id = $2",
                    [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                    newHash, user_id);
                std::cout << "Migrated password hash to PBKDF2 for user: " << email << std::endl;
            }

            bool mfa_enabled = result[0]["mfa_enabled"].as<bool>();
            bool email_verified = result[0]["email_verified"].as<bool>();
            
            std::string token = generateJWT(user_id, mfa_enabled);

            Json::Value ret;
            if (mfa_enabled) {
                ret["status"] = "mfa_required";
                ret["mfa_token"] = token;  // MFA tokens are short-lived, safe in body
            } else {
                ret["status"] = "success";
                ret["user_id"] = user_id;
            }
            ret["email"] = email;
            ret["email_verified"] = email_verified;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");

            if (!mfa_enabled) {
                setAuthCookie(resp, token);
            }
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        email
    );
}

// ─── MFA Login ───────────────────────────────────────────────────────────────

void AuthController::mfaLoginUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("mfa_token") || !jsonPtr->isMember("code")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing mfa_token or code");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string mfa_token = (*jsonPtr)["mfa_token"].asString();
    std::string code = (*jsonPtr)["code"].asString();

    std::string user_id;
    try {
        auto decoded = jwt::decode(mfa_token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{getJwtSecret()})
            .with_issuer("bankara_mfa_pending");
        verifier.verify(decoded);
        user_id = decoded.get_payload_claim("user_id").as_string();
    } catch (const std::exception& e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid MFA token");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT mfa_secret, email_verified, email FROM users WHERE id = $1",
        [callback, this, user_id, code](const drogon::orm::Result &result) {
            if (result.empty() || result[0]["mfa_secret"].isNull()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("MFA not set up");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string secret = result[0]["mfa_secret"].as<std::string>();
            if (Totp::verify(secret, code)) {
                bool email_verified = result[0]["email_verified"].as<bool>();
                std::string email = result[0]["email"].as<std::string>();
                std::string token = generateJWT(user_id, false);
                
                Json::Value ret;
                ret["status"] = "success";
                ret["user_id"] = user_id;
                ret["email_verified"] = email_verified;
                ret["email"] = email;
                
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                setAuthCookie(resp, token);
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid MFA code");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
            }
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error.");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

// ─── Logout ──────────────────────────────────────────────────────────────────

void AuthController::logoutUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    Json::Value ret;
    ret["status"] = "success";
    ret["message"] = "Logged out successfully";
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    clearAuthCookie(resp);
    callback(resp);
}

// ─── Delete Account ──────────────────────────────────────────────────────────

void AuthController::deleteAccount(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing password");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }
    std::string password = (*jsonPtr)["password"].asString();

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT password_hash, failed_login_attempts FROM users WHERE id = $1",
        [callback, this, password, dbClient, user_id](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("User not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string stored_hash = result[0]["password_hash"].as<std::string>();
            int failed_attempts = result[0]["failed_login_attempts"].as<int>();

            bool passwordValid = false;
            if (stored_hash.find(':') != std::string::npos) {
                passwordValid = verifyPasswordPBKDF2(password, stored_hash);
            } else {
                std::string legacyHash = hashPassword(password);
                if (legacyHash == stored_hash) passwordValid = true;
            }

            if (!passwordValid) {
                failed_attempts++;
                int cooldown_minutes = 0;
                if (failed_attempts >= 7) cooldown_minutes = 10;
                else if (failed_attempts >= 5) cooldown_minutes = 5;
                else if (failed_attempts >= 3) cooldown_minutes = 1;

                if (cooldown_minutes > 0) {
                    dbClient->execSqlAsync(
                        "UPDATE users SET failed_login_attempts = $1, lockout_until = now() + make_interval(mins => $2) WHERE id = $3",
                        [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                        failed_attempts, cooldown_minutes, user_id);
                    
                    Json::Value ret;
                    ret["status"] = "locked";
                    ret["message"] = "Account locked due to multiple failed attempts. You have been logged out.";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k429TooManyRequests);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    clearAuthCookie(resp);
                    callback(resp);
                } else {
                    dbClient->execSqlAsync(
                        "UPDATE users SET failed_login_attempts = $1 WHERE id = $2",
                        [](const drogon::orm::Result &){}, [](const drogon::orm::DrogonDbException &){},
                        failed_attempts, user_id);
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k401Unauthorized);
                    resp->setBody("Invalid password");
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                }
                return;
            }

            // Correct password - proceed to delete (soft delete)
            dbClient->execSqlAsync(
                "UPDATE users SET password_hash = 'DELETED', mfa_secret = '', email = email || ' [deleted-' || id || ']' WHERE id = $1",
                [callback, this](const drogon::orm::Result &) {
                    Json::Value ret;
                    ret["status"] = "success";
                    ret["message"] = "Account successfully deleted";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    clearAuthCookie(resp);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody("Database error during user deletion");
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                },
                user_id
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

// ─── Verify Email ────────────────────────────────────────────────────────────

void AuthController::verifyEmail(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("token")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing verification token");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string token = (*jsonPtr)["token"].asString();
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "UPDATE users SET email_verified = TRUE, verification_token = NULL WHERE verification_token = $1 RETURNING id",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid or expired verification token");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            Json::Value ret;
            ret["status"] = "success";
            ret["message"] = "Email verified successfully";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        token
    );
}

// ─── Get Profile ─────────────────────────────────────────────────────────────

void AuthController::getProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT email, mfa_enabled, email_verified FROM users WHERE id = $1",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("User not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            Json::Value ret;
            ret["email"] = result[0]["email"].as<std::string>();
            ret["mfa_enabled"] = result[0]["mfa_enabled"].as<bool>();
            ret["email_verified"] = result[0]["email_verified"].as<bool>();
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

// ─── Resend Verification ─────────────────────────────────────────────────────

void AuthController::resendVerification(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto dbClient = drogon::app().getDbClient();
    std::string verification_token = drogon::utils::getUuid();

    dbClient->execSqlAsync(
        "UPDATE users SET verification_token = $1 WHERE id = $2 AND email_verified = FALSE RETURNING email",
        [callback, verification_token](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Email already verified or user not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            std::string email = result[0]["email"].as<std::string>();
            std::cout << "Verification link for " << email << ": http://localhost:5150/verify-email?token=" << verification_token << std::endl;
            
            Json::Value ret;
            ret["status"] = "success";
            ret["message"] = "Verification email resent";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        verification_token, user_id
    );
}

// ─── Forgot Password ─────────────────────────────────────────────────────────

void AuthController::forgotPassword(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("email")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string email = (*jsonPtr)["email"].asString();
    std::string reset_token = drogon::utils::getUuid();
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "UPDATE users SET password_reset_token = $1, password_reset_expires = now() + interval '15 minutes' "
        "WHERE email = $2 AND password_hash != '' RETURNING id",
        [callback, reset_token, email](const drogon::orm::Result &result) {
            // Always return success to prevent email enumeration
            Json::Value ret;
            ret["status"] = "success";
            ret["message"] = "If an account with that email exists, a password reset link has been sent.";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");

            if (!result.empty()) {
                std::cout << "Password reset link for " << email << ": http://localhost:5150/reset-password?token=" << reset_token << std::endl;
            }
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        reset_token, email
    );
}

// ─── Reset Password ──────────────────────────────────────────────────────────

void AuthController::resetPassword(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("token") || !jsonPtr->isMember("new_password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing token or new_password");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string token = (*jsonPtr)["token"].asString();
    std::string new_password = (*jsonPtr)["new_password"].asString();

    std::string pwError;
    if (!validatePasswordStrength(new_password, pwError)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(pwError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string new_hash = hashPasswordPBKDF2(new_password);
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "UPDATE users SET password_hash = $1, password_reset_token = NULL, password_reset_expires = NULL "
        "WHERE password_reset_token = $2 AND password_reset_expires > now() RETURNING id",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid or expired reset token");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            Json::Value ret;
            ret["status"] = "success";
            ret["message"] = "Password has been reset successfully";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        new_hash, token
    );
}

// ─── Request Password Change (authenticated) ────────────────────────────────

void AuthController::requestPasswordChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("current_password") || !jsonPtr->isMember("new_password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing current_password or new_password");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string current_password = (*jsonPtr)["current_password"].asString();
    std::string new_password = (*jsonPtr)["new_password"].asString();

    std::string pwError;
    if (!validatePasswordStrength(new_password, pwError)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(pwError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT password_hash, email FROM users WHERE id = $1",
        [callback, this, current_password, new_password, user_id, dbClient](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("User not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string stored_hash = result[0]["password_hash"].as<std::string>();
            std::string email = result[0]["email"].as<std::string>();

            // Verify current password
            bool currentValid = false;
            if (stored_hash.find(':') != std::string::npos) {
                currentValid = verifyPasswordPBKDF2(current_password, stored_hash);
            } else {
                currentValid = (hashPassword(current_password) == stored_hash);
            }

            if (!currentValid) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Current password is incorrect");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            // Generate pending hash and confirmation token
            std::string pending_hash = hashPasswordPBKDF2(new_password);
            std::string change_token = drogon::utils::getUuid();

            dbClient->execSqlAsync(
                "UPDATE users SET pending_password_hash = $1, password_change_token = $2 WHERE id = $3",
                [callback, change_token, email](const drogon::orm::Result &) {
                    std::cout << "Password change confirmation for " << email << ": http://localhost:5150/confirm-password-change?token=" << change_token << std::endl;

                    Json::Value ret;
                    ret["status"] = "success";
                    ret["message"] = "A confirmation link has been printed to the server console. Click it to finalize the password change.";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody("Database error");
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                },
                pending_hash, change_token, user_id
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

// ─── Confirm Password Change ─────────────────────────────────────────────────

void AuthController::confirmPasswordChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("token")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing token");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string token = (*jsonPtr)["token"].asString();
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "UPDATE users SET password_hash = pending_password_hash, pending_password_hash = NULL, password_change_token = NULL "
        "WHERE password_change_token = $1 AND pending_password_hash IS NOT NULL RETURNING id",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid or expired password change token");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }
            Json::Value ret;
            ret["status"] = "success";
            ret["message"] = "Password changed successfully";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        token
    );
}

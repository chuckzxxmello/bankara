#include "MfaController.h"
#include "Totp.h"
#include <drogon/drogon.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>

void MfaController::setup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
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

    dbClient->execSqlAsync(
        "SELECT email, mfa_enabled FROM users WHERE id = $1",
        [callback, user_id, dbClient](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("User not found");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            if (result[0]["mfa_enabled"].as<bool>()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("MFA is already enabled");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string email = result[0]["email"].as<std::string>();
            std::string secret = Totp::generateSecret();

            dbClient->execSqlAsync(
                "UPDATE users SET mfa_secret = $1 WHERE id = $2",
                [callback, secret, email](const drogon::orm::Result &updateRes) {
                    Json::Value ret;
                    ret["secret"] = secret;
                    ret["uri"] = "otpauth://totp/Bankara:" + email + "?secret=" + secret + "&issuer=Bankara";
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
                secret, user_id
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

void MfaController::verify(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("code")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing MFA code");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string code = (*jsonPtr)["code"].asString();
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT mfa_secret FROM users WHERE id = $1",
        [callback, code, user_id, dbClient](const drogon::orm::Result &result) {
            if (result.empty() || result[0]["mfa_secret"].isNull()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("MFA setup not initiated");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string secret = result[0]["mfa_secret"].as<std::string>();
            if (Totp::verify(secret, code)) {
                dbClient->execSqlAsync(
                    "UPDATE users SET mfa_enabled = TRUE WHERE id = $1",
                    [callback](const drogon::orm::Result &updateRes) {
                        Json::Value ret;
                        ret["status"] = "success";
                        ret["message"] = "MFA enabled successfully";
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
            resp->setBody("Database error");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        },
        user_id
    );
}

void MfaController::status(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
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
        "SELECT mfa_enabled FROM users WHERE id = $1",
        [callback](const drogon::orm::Result &result) {
            Json::Value ret;
            if (result.empty()) {
                ret["mfa_enabled"] = false;
            } else {
                ret["mfa_enabled"] = result[0]["mfa_enabled"].as<bool>();
            }
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

void MfaController::disableMfa(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const {
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Password is required to disable MFA");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }
    std::string password = (*jsonPtr)["password"].asString();
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT password_hash FROM users WHERE id = $1",
        [callback, password, user_id, dbClient](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            std::string stored_hash = result[0]["password_hash"].as<std::string>();
            bool valid = false;

            // Simple standalone verify for PBKDF2
            auto colonPos = stored_hash.find(':');
            if (colonPos != std::string::npos) {
                std::string saltHex = stored_hash.substr(0, colonPos);
                std::string hashHex = stored_hash.substr(colonPos + 1);

                std::vector<unsigned char> saltBytes;
                for (size_t i = 0; i < saltHex.length(); i += 2) {
                    saltBytes.push_back((unsigned char)strtol(saltHex.substr(i, 2).c_str(), nullptr, 16));
                }
                std::vector<unsigned char> storedHashBytes;
                for (size_t i = 0; i < hashHex.length(); i += 2) {
                    storedHashBytes.push_back((unsigned char)strtol(hashHex.substr(i, 2).c_str(), nullptr, 16));
                }

                unsigned char derived[32];
                PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                                   saltBytes.data(), saltBytes.size(), 100000,
                                   EVP_sha256(), 32, derived);

                if (storedHashBytes.size() == 32) {
                    unsigned char res = 0;
                    for (size_t i = 0; i < 32; i++) {
                        res |= derived[i] ^ storedHashBytes[i];
                    }
                    valid = (res == 0);
                }
            } else {
                // Legacy SHA-256 fallback for migration
                unsigned char legacyHash[SHA256_DIGEST_LENGTH];
                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, password.c_str(), password.size());
                SHA256_Final(legacyHash, &sha256);
                
                std::stringstream ss;
                for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)legacyHash[i];
                }
                valid = (ss.str() == stored_hash);
            }

            if (!valid) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid password");
                resp->addHeader("Access-Control-Allow-Origin", "*");
                callback(resp);
                return;
            }

            dbClient->execSqlAsync(
                "UPDATE users SET mfa_enabled = FALSE, mfa_secret = NULL WHERE id = $1",
                [callback](const drogon::orm::Result &result) {
                    Json::Value ret;
                    ret["status"] = "success";
                    ret["message"] = "MFA disabled successfully";
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

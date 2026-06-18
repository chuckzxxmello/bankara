#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", Post, Options);
    ADD_METHOD_TO(AuthController::loginUser, "/api/auth/login", Post, Options);
    ADD_METHOD_TO(AuthController::mfaLoginUser, "/api/auth/mfa-login", Post, Options);
    ADD_METHOD_TO(AuthController::logoutUser, "/api/auth/logout", Post, Options);
    ADD_METHOD_TO(AuthController::deleteAccount, "/api/auth/delete-account", Delete, Options, "JwtFilter");
    ADD_METHOD_TO(AuthController::verifyEmail, "/api/auth/verify-email", Post, Options);
    ADD_METHOD_TO(AuthController::resendVerification, "/api/auth/resend-verification", Post, Options, "JwtFilter");
    ADD_METHOD_TO(AuthController::getProfile, "/api/auth/profile", Get, Options, "JwtFilter");
    ADD_METHOD_TO(AuthController::forgotPassword, "/api/auth/forgot-password", Post, Options);
    ADD_METHOD_TO(AuthController::resetPassword, "/api/auth/reset-password", Post, Options);
    ADD_METHOD_TO(AuthController::requestPasswordChange, "/api/auth/request-password-change", Post, Options, "JwtFilter");
    ADD_METHOD_TO(AuthController::confirmPasswordChange, "/api/auth/confirm-password-change", Post, Options);
    METHOD_LIST_END

    void registerUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void loginUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void mfaLoginUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void logoutUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void deleteAccount(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void verifyEmail(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void resendVerification(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void getProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void forgotPassword(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void resetPassword(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void requestPasswordChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void confirmPasswordChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;

private:
    std::string hashPassword(const std::string& password) const;  // legacy SHA-256
    std::string hashPasswordPBKDF2(const std::string& password) const;
    bool verifyPasswordPBKDF2(const std::string& password, const std::string& stored) const;
    std::string generateJWT(const std::string& userId, bool mfa_pending = false) const;
    std::string getJwtSecret() const;
    void setAuthCookie(const HttpResponsePtr& resp, const std::string& token) const;
    void clearAuthCookie(const HttpResponsePtr& resp) const;
};

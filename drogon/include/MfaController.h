#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class MfaController : public drogon::HttpController<MfaController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MfaController::setup, "/api/mfa/setup", Post, Options, "JwtFilter");
    ADD_METHOD_TO(MfaController::verify, "/api/mfa/verify", Post, Options, "JwtFilter");
    ADD_METHOD_TO(MfaController::status, "/api/mfa/status", Get, Options, "JwtFilter");
    ADD_METHOD_TO(MfaController::disableMfa, "/api/mfa/disable", Post, Options, "JwtFilter");
    METHOD_LIST_END

    void setup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void verify(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void status(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    void disableMfa(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
};

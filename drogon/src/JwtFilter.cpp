#include "JwtFilter.h"
#include <jwt-cpp/jwt.h>

static std::string getJwtSecret() {
    const char* env = std::getenv("JWT_SECRET");
    if (env && strlen(env) > 0) {
        return std::string(env);
    }
    throw std::runtime_error("FATAL: JWT_SECRET environment variable is not set!");
}

void JwtFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    if (req->method() == Options) {
        fccb();
        return;
    }

    std::string token;

    // 1. Try HttpOnly cookie first (preferred, XSS-safe)
    token = req->getCookie("bankara_token");

    // 2. Fallback to Authorization header (for backward compat / API clients)
    if (token.empty()) {
        auto authHeader = req->getHeader("Authorization");
        if (!authHeader.empty() && authHeader.substr(0, 7) == "Bearer ") {
            token = authHeader.substr(7);
        }
    }

    if (token.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Missing or invalid authentication");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        fcb(resp);
        return;
    }

    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{getJwtSecret()})
            .with_issuer("bankara_auth");

        verifier.verify(decoded);

        std::string user_id = decoded.get_payload_claim("user_id").as_string();
        req->getAttributes()->insert("user_id", user_id);

        fccb(); // Success
    } catch (const std::exception& e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody(std::string("Token error: ") + e.what());
        resp->addHeader("Access-Control-Allow-Origin", "*");
        fcb(resp);
    }
}

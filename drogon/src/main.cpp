#include <drogon/drogon.h>
#include <iostream>

using namespace std;

int main() {
    cout << "Running Server on http://localhost:5150" << endl;

    auto fallbackResp = drogon::HttpResponse::newFileResponse("./dist/index.html");

    drogon::app()
        .loadConfigFile("config.json")
        .setCustom404Page(fallbackResp)
        // Global security headers on ALL responses
        .registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
                resp->addHeader("X-Frame-Options", "DENY");
                resp->addHeader("X-Content-Type-Options", "nosniff");
                resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
                resp->addHeader("X-XSS-Protection", "1; mode=block");
                resp->addHeader("Content-Security-Policy",
                    "default-src 'self'; "
                    "script-src 'self'; "
                    "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; "
                    "font-src 'self' https://fonts.gstatic.com; "
                    "img-src 'self' data:; "
                    "connect-src 'self'");
            })
        .run();

    return 0;
}

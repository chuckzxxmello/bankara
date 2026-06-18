#include "WeatherForecastController.h"

void WeatherForecastController::get(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const
{
    Json::Value ret;
    ret["date"] = "2026-06-17";
    ret["temperatureC"] = 25;
    ret["summary"] = "Mild";

    auto resp = HttpResponse::newHttpJsonResponse(ret);
    // Add CORS headers if needed, though Vite proxy handles this usually
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

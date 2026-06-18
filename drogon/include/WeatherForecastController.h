#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class WeatherForecastController : public drogon::HttpController<WeatherForecastController>
{
public:
    METHOD_LIST_BEGIN
    // Use METHOD_ADD to define routes, similar to [HttpGet("api/weatherforecast")] in .NET
    ADD_METHOD_TO(WeatherForecastController::get, "/api/weatherforecast", Get);
    METHOD_LIST_END

    void get(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
};

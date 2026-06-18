#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class IndexController : public drogon::HttpController<IndexController> {
public:
    METHOD_LIST_BEGIN
        METHOD_ADD(IndexController::index, "/");
    METHOD_LIST_END

        void index(const HttpRequestPtr& req,
            std::function<void(const HttpResponsePtr&)>&& callback) {
        // Serve the index.html from the build folder
        auto res = HttpResponse::newFileResponse("./frontend/build/index.html");
        callback(res);
    }
};

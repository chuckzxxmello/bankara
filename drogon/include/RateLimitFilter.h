#pragma once
#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

using namespace drogon;

class RateLimitFilter : public HttpFilter<RateLimitFilter>
{
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

private:
    struct RequestLog {
        std::vector<std::chrono::steady_clock::time_point> timestamps;
    };

    static std::unordered_map<std::string, RequestLog> requestMap_;
    static std::mutex mutex_;
    static std::chrono::steady_clock::time_point lastCleanup_;

    static constexpr int MAX_REQUESTS_PER_MINUTE = 100;
    static constexpr int CLEANUP_INTERVAL_SECONDS = 60;
};

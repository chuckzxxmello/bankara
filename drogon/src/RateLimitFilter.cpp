#include "RateLimitFilter.h"
#include <algorithm>

std::unordered_map<std::string, RateLimitFilter::RequestLog> RateLimitFilter::requestMap_;
std::mutex RateLimitFilter::mutex_;
std::chrono::steady_clock::time_point RateLimitFilter::lastCleanup_ = std::chrono::steady_clock::now();

void RateLimitFilter::doFilter(const HttpRequestPtr &req,
                                FilterCallback &&fcb,
                                FilterChainCallback &&fccb)
{
    // Skip rate limiting for OPTIONS (CORS preflight)
    if (req->method() == Options) {
        fccb();
        return;
    }

    std::string clientIp = req->getPeerAddr().toIp();
    std::string xff = req->getHeader("X-Forwarded-For");
    if (!xff.empty()) {
        size_t commaPos = xff.find(',');
        if (commaPos != std::string::npos) {
            clientIp = xff.substr(0, commaPos);
        } else {
            clientIp = xff;
        }
    }
    auto now = std::chrono::steady_clock::now();
    auto oneMinuteAgo = now - std::chrono::seconds(60);

    std::lock_guard<std::mutex> lock(mutex_);

    // Periodic cleanup of stale entries
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup_).count() > CLEANUP_INTERVAL_SECONDS) {
        for (auto it = requestMap_.begin(); it != requestMap_.end();) {
            // Remove timestamps older than 1 minute
            auto& ts = it->second.timestamps;
            ts.erase(std::remove_if(ts.begin(), ts.end(),
                [&oneMinuteAgo](const auto& t) { return t < oneMinuteAgo; }), ts.end());
            if (ts.empty()) {
                it = requestMap_.erase(it);
            } else {
                ++it;
            }
        }
        lastCleanup_ = now;
    }

    auto& log = requestMap_[clientIp];

    // Remove expired timestamps for this IP
    log.timestamps.erase(
        std::remove_if(log.timestamps.begin(), log.timestamps.end(),
            [&oneMinuteAgo](const auto& t) { return t < oneMinuteAgo; }),
        log.timestamps.end());

    if ((int)log.timestamps.size() >= MAX_REQUESTS_PER_MINUTE) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k429TooManyRequests);
        resp->setBody("Rate limit exceeded. Please try again later.");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Retry-After", "60");
        fcb(resp);
        return;
    }

    log.timestamps.push_back(now);
    fccb();
}

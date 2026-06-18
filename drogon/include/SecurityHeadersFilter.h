#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class SecurityHeadersFilter : public HttpFilter<SecurityHeadersFilter>
{
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};

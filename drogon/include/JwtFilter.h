#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class JwtFilter : public HttpFilter<JwtFilter>
{
public:
    virtual void doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) override;
};

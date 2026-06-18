#include "SecurityHeadersFilter.h"

void SecurityHeadersFilter::doFilter(const HttpRequestPtr &req,
                                      FilterCallback &&fcb,
                                      FilterChainCallback &&fccb)
{
    // Pass through to controller, then we add headers via a global AOP-style approach.
    // Since Drogon filters run BEFORE the handler, we use a post-routing approach instead.
    // We'll add headers globally via main.cpp registerPostHandlingAdvice.
    fccb();
}

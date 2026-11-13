#include "platform/clock.h"
#include "platform/host/clock_host.h"

namespace {
uint32_t g_nowMs = 0;
}

namespace platform {

uint32_t nowMs() {
    return g_nowMs;
}

namespace host {

void setNowMs(uint32_t nowMs) {
    g_nowMs = nowMs;
}

void advanceMs(uint32_t deltaMs) {
    g_nowMs += deltaMs;
}

}  // namespace host
}  // namespace platform

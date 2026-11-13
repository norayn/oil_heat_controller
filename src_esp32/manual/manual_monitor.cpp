#include "manual/manual_monitor.h"

#include "config/limits.h"
#include "platform/log.h"

void ManualMonitor::begin(uint32_t nowMs) {
    timer_.start(nowMs, kManualMonitorPeriodMs);
    started_ = true;
}

void ManualMonitor::tick(uint32_t nowMs, ActuatorBits applied, bool outputsUnknown) {
    if (!started_ || !timer_.due(nowMs)) {
        return;
    }

    platform::logf(platform::LogLevel::Info, "MON",
                   "applied=0x%04x outputsUnknown=%d",
                   static_cast<unsigned>(applied),
                   outputsUnknown ? 1 : 0);
}

#pragma once
// Периодический лог manual-режима (500 мс). НЕ interlock: не пишет выходы,
// его отказ не меняет поведение.

#include <cstdint>
#include "domain/actuator_types.h"
#include "services/scheduler.h"

class ManualMonitor {
public:
    void begin(uint32_t nowMs);
    void tick(uint32_t nowMs, ActuatorBits applied, bool outputsUnknown);

private:
    PeriodicTimer timer_;
    bool          started_{false};
};

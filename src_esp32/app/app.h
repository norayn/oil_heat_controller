#pragma once
// Composition root и общий контракт приложения.
// app_common.cpp — только profile-agnostic wiring; конкретную сборку
// реализуют app_field.cpp / app_manual.cpp / app_bench_auto.cpp.

#include "domain/safety_types.h"

class ActuatorService;
class Scheduler;

namespace app {

void setup();
void tick();

ActuatorService& actuators();
Scheduler& scheduler();
SafetyConfig safetyConfig();

// Реализуется ровно одним composition root (build_src_filter).
const char* profileName();
void setupProfile();
void tickProfile();

}  // namespace app

// BENCH-AUTO composition root (esp32_bench_auto).
// FIELD-ядро + NanoStubLink вместо SafetyLink: «Nano говорит всё ОК» (AR-05).
// FSM/interlocks появятся в M4/M4b; M0 проверяет только сборку и lease-путь.

#include "app/app.h"

#include "bench/nano_stub_link.h"
#include "domain/mode_manager.h"
#include "domain/safety_supervisor.h"
#include "platform/clock.h"
#include "platform/log.h"
#include "services/actuator_service.h"

namespace {

NanoStubLink     g_safety;
SafetySupervisor g_supervisor;
ModeManager      g_mode;
FaultBits        g_localFaults = F_NONE;

}  // namespace

namespace app {

const char* profileName() {
    return NanoStubLink::kBanner;
}

void setupProfile() {
    const uint32_t now = platform::nowMs();

    g_supervisor.reset(now);
    g_mode.begin(now);
    g_mode.finishSelfCheck(now, true);

    platform::logf(platform::LogLevel::Warn, "BENCH",
                   "заглушка Nano активна с загрузки; защит нет");
}

void tickProfile() {
    const uint32_t now = platform::nowMs();

    g_safety.poll(now);
    const SafetyDecision decision = g_supervisor.evaluate(
        g_safety.lease(), g_localFaults, now, safetyConfig());

    if (decision.latchRequest) {
        g_mode.recordLatch(now);
    } else if (g_mode.mode() == RunMode::Off && decision.permitProcess) {
        g_mode.requestAuto(now, decision.permitProcess, false);
    }

    ActuatorDemand demand;
    demand.state = BurnerState::Idle;
    demand.bits = kAllOff;
    demand.permitDangerous = decision.permitDangerous;
    demand.permitHeater = decision.permitHeater;
    demand.allowHeatOil = false;

    actuators().tickAuto(now, demand);
}

}  // namespace app

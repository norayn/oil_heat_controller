// FIELD composition root (esp32_core).
// M0: supervisor + NullSafetySource; FSM/interlocks/retry появятся в M4.
// Выходы всегда OFF, AUTO недостижим до реального lease (M3b).

#include "app/app.h"

#include "app/null_manual_request.h"
#include "domain/mode_manager.h"
#include "domain/safety_supervisor.h"
#include "platform/clock.h"
#include "platform/log.h"
#include "services/actuator_service.h"
#include "services/null_safety_source.h"

namespace {

NullSafetySource g_safety;
SafetySupervisor g_supervisor;
ModeManager      g_mode;
NullManualRequest g_manualRequest;
FaultBits        g_localFaults = F_NONE;

BurnerState stateFromMode(RunMode mode) {
    switch (mode) {
        case RunMode::BootSafe: return BurnerState::Init;
        case RunMode::Fault:
        case RunMode::Latched:  return BurnerState::Fault;
        case RunMode::Off:
        case RunMode::Manual:
        case RunMode::Auto:     return BurnerState::Idle;
    }
    return BurnerState::Fault;
}

}  // namespace

namespace app {

const char* profileName() {
    return "FIELD (esp32_core)";
}

void setupProfile() {
    const uint32_t now = platform::nowMs();

    g_supervisor.reset(now);
    g_mode.begin(now);
    g_mode.finishSelfCheck(now, true);

    // В FIELD порт всегда false: перехода в Manual нет.
    (void)g_manualRequest.requested();

    platform::logf(platform::LogLevel::Info, "FIELD",
                   "mode=%s; AUTO недоступен до Nano link (M3b)",
                   runModeName(g_mode.mode()));
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

    ActuatorDemand demand;   // fail-closed defaults
    demand.state = stateFromMode(g_mode.mode());
    demand.bits = kAllOff;
    demand.permitDangerous = decision.permitDangerous;
    demand.permitHeater = decision.permitHeater;
    demand.allowHeatOil = false;

    actuators().tickAuto(now, demand);
}

}  // namespace app

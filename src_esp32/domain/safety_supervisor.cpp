#include "domain/safety_supervisor.h"
#include "domain/safety_policy.h"

void SafetySupervisor::reset(uint32_t nowMs) {
    (void)nowMs;
    history_ = SafetyHistory{};
}

SafetyDecision SafetySupervisor::evaluate(const SafetyLease& lease,
                                          FaultBits localFaults,
                                          uint32_t nowMs,
                                          const SafetyConfig& cfg) {
    // История обновляется до оценки: "ever" означает "когда-либо, включая
    // текущий кадр". Для ветки !permit это безопасно, потому что permit=true
    // в этом кадре не может одновременно попасть в ветку "разрешение потеряно".
    if (leaseFresh(lease, nowMs)) {
        history_.everValid = true;
        if (lease.permit) {
            history_.everPermitted = true;
        }
        if (lease.contactorFb == ContactorFb::OnConfirmed) {
            history_.everContactorConfirmed = true;
        }
        if (lease.kc868PowerFb == PowerFb::Present) {
            history_.everPowerConfirmed = true;
        }
    }

    return safetyPolicyEvaluate(lease, history_, localFaults, nowMs, cfg);
}

#pragma once
// Единственный stateful-арбитр безопасности (ADR-15).
// Владеет SafetyHistory (ever*) и передаёт её в чистую SafetyPolicy.
// В manual-сборку не входит; в FIELD создаётся только composition root.

#include <cstdint>
#include "domain/safety_types.h"

class SafetySupervisor {
public:
    void reset(uint32_t nowMs);

    SafetyDecision evaluate(const SafetyLease& lease,
                            FaultBits localFaults,
                            uint32_t nowMs,
                            const SafetyConfig& cfg);

    const SafetyHistory& history() const { return history_; }

private:
    SafetyHistory history_{};
};

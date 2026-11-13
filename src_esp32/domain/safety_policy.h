#pragma once
// Чистая функция политики безопасности (ADR-15, §7.3).
// Не хранит состояния, не пишет актуаторы, не включает common_protocol.h.
// Host-тестируема; единственный stateful-арбитр — SafetySupervisor.

#include <cstdint>
#include "domain/safety_types.h"

SafetyDecision safetyPolicyEvaluate(const SafetyLease& lease,
                                    const SafetyHistory& history,
                                    const FaultBits& localFaults,
                                    uint32_t nowMs,
                                    const SafetyConfig& cfg);

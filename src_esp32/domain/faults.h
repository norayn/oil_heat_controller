#pragma once
// Fault-биты и их severity. CRITICAL/HIGH — липкие до явного reset (§7.5).
// Заголовок host-clean: без <Arduino.h>, без HAL, без глобального времени.

#include <cstdint>
#include "domain/burner_types.h"

using FaultBits = uint16_t;

constexpr FaultBits F_NONE                 = 0;
constexpr FaultBits F_LINK_LOSS            = 1u << 0;
constexpr FaultBits F_PERMIT_LOST          = 1u << 1;
constexpr FaultBits F_SAFETY_ALARM         = 1u << 2;
constexpr FaultBits F_GATE_FAULT           = 1u << 3;
constexpr FaultBits F_SENSOR_INVALID       = 1u << 4;
constexpr FaultBits F_FLAME_SENSOR_INVALID = 1u << 5;
constexpr FaultBits F_FLAME_LOSS           = 1u << 6;
constexpr FaultBits F_EXHAUST_INVALID      = 1u << 7;
constexpr FaultBits F_I2C_FAULT            = 1u << 8;
constexpr FaultBits F_ACTUATOR_MISMATCH    = 1u << 9;
constexpr FaultBits F_TIMEOUT              = 1u << 10;
constexpr FaultBits F_IGNITION_TIMEOUT     = 1u << 11;
constexpr FaultBits F_INTERNAL             = 1u << 12;

constexpr FaultBits kAllFaults = (FaultBits(1u) << 13) - 1u;

constexpr FaultBits kCriticalFaultMask = F_I2C_FAULT | F_INTERNAL;

constexpr FaultBits kHighFaultMask =
    F_LINK_LOSS | F_PERMIT_LOST | F_SAFETY_ALARM | F_GATE_FAULT |
    F_SENSOR_INVALID | F_FLAME_SENSOR_INVALID | F_FLAME_LOSS |
    F_EXHAUST_INVALID | F_ACTUATOR_MISMATCH | F_TIMEOUT;

constexpr FaultBits kMediumFaultMask = F_IGNITION_TIMEOUT;
constexpr FaultBits kLowFaultMask    = F_NONE;

// CRITICAL/HIGH — только явный reset; по ним SafetyPolicy выдаёт Latch.
constexpr FaultBits kLatchFaultMask = kCriticalFaultMask | kHighFaultMask;

enum class FaultSeverity : uint8_t { None = 0, Low, Medium, High, Critical };

constexpr FaultSeverity faultSeverity(FaultBits faults) {
    if (faults & kCriticalFaultMask) return FaultSeverity::Critical;
    if (faults & kHighFaultMask)     return FaultSeverity::High;
    if (faults & kMediumFaultMask)   return FaultSeverity::Medium;
    if (faults & kLowFaultMask)      return FaultSeverity::Low;
    return FaultSeverity::None;
}

constexpr const char* faultSeverityName(FaultSeverity severity) {
    switch (severity) {
        case FaultSeverity::None:     return "NONE";
        case FaultSeverity::Low:      return "LOW";
        case FaultSeverity::Medium:   return "MEDIUM";
        case FaultSeverity::High:     return "HIGH";
        case FaultSeverity::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

// Причина останова по старшему fault-биту. Нужна для диагностики и retry-политики.
constexpr StopReason stopReasonFromFaults(FaultBits faults) {
    if (faults & F_FLAME_LOSS)           return StopReason::FlameLoss;
    if (faults & F_FLAME_SENSOR_INVALID) return StopReason::FlameSensorInvalid;
    if (faults & F_EXHAUST_INVALID)      return StopReason::ExhaustInvalid;
    if (faults & F_LINK_LOSS)            return StopReason::LinkLoss;
    if (faults & F_PERMIT_LOST)          return StopReason::PermitLost;
    if (faults & F_GATE_FAULT)           return StopReason::GateFault;
    if (faults & F_SAFETY_ALARM)         return StopReason::SafetyAlarm;
    if (faults & F_I2C_FAULT)            return StopReason::I2cFault;
    if (faults & F_ACTUATOR_MISMATCH)    return StopReason::ActuatorMismatch;
    if (faults & F_SENSOR_INVALID)       return StopReason::SensorInvalid;
    if (faults & F_TIMEOUT)              return StopReason::Timeout;
    if (faults & F_IGNITION_TIMEOUT)     return StopReason::IgnitionTimeout;
    return StopReason::Internal;
}

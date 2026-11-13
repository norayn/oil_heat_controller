#pragma once
// Состояния горелки и причины останова.
// Заголовок host-clean: без <Arduino.h>, без #if OILCTRL_*, без HAL.

#include <cstdint>

enum class BurnerState : uint8_t {
    Init = 0,
    Idle,
    PrePurge,
    Ignition,
    Run,
    PostPurge,
    Shutdown,   // reserved: power-down, в M0–M4 недостижим
    Fault,
    Latched
};

enum class StopReason : uint8_t {
    None = 0,
    HeatDone,
    UserOff,
    TrunMax,
    PermitLost,
    LinkLoss,
    SafetyAlarm,
    GateFault,
    SensorInvalid,
    FlameSensorInvalid,
    FlameLoss,
    ExhaustInvalid,
    IgnitionTimeout,
    Timeout,
    I2cFault,
    ActuatorMismatch,
    Internal
};

constexpr const char* burnerStateName(BurnerState state) {
    switch (state) {
        case BurnerState::Init:      return "Init";
        case BurnerState::Idle:      return "Idle";
        case BurnerState::PrePurge:  return "PrePurge";
        case BurnerState::Ignition:  return "Ignition";
        case BurnerState::Run:       return "Run";
        case BurnerState::PostPurge: return "PostPurge";
        case BurnerState::Shutdown:  return "Shutdown";
        case BurnerState::Fault:     return "Fault";
        case BurnerState::Latched:   return "Latched";
    }
    return "UNKNOWN";
}

constexpr const char* stopReasonName(StopReason reason) {
    switch (reason) {
        case StopReason::None:               return "None";
        case StopReason::HeatDone:           return "HeatDone";
        case StopReason::UserOff:            return "UserOff";
        case StopReason::TrunMax:            return "TrunMax";
        case StopReason::PermitLost:         return "PermitLost";
        case StopReason::LinkLoss:           return "LinkLoss";
        case StopReason::SafetyAlarm:        return "SafetyAlarm";
        case StopReason::GateFault:          return "GateFault";
        case StopReason::SensorInvalid:      return "SensorInvalid";
        case StopReason::FlameSensorInvalid: return "FlameSensorInvalid";
        case StopReason::FlameLoss:          return "FlameLoss";
        case StopReason::ExhaustInvalid:     return "ExhaustInvalid";
        case StopReason::IgnitionTimeout:    return "IgnitionTimeout";
        case StopReason::Timeout:            return "Timeout";
        case StopReason::I2cFault:           return "I2cFault";
        case StopReason::ActuatorMismatch:   return "ActuatorMismatch";
        case StopReason::Internal:           return "Internal";
    }
    return "UNKNOWN";
}

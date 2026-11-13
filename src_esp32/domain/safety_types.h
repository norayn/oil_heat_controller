#pragma once
// Доменные типы lease/permit и решение политики безопасности (§7.3).
// Здесь НЕТ include/common_protocol.h: маппинг protocol -> domain живёт
// только в SafetyLink и доказывается static_assert'ами.
// Host-clean: без <Arduino.h>, без HAL, без глобального времени.

#include <cstdint>
#include "config/limits.h"
#include "domain/burner_types.h"
#include "domain/faults.h"

enum class GateState : uint8_t { Unknown = 0, Closed, OpenSafe, Latched, Fault };
enum class ContactorFb : uint8_t { Unknown = 0, Off, OnConfirmed, Fault };
enum class PowerFb : uint8_t { Unknown = 0, Present, Absent, Fault };

using AlarmBits       = uint16_t;
using SensorValidBits = uint16_t;

// Доменные биты валидности. SafetyLink обязан доказать static_assert'ом,
// что они совпадают с protocol-битами.
constexpr SensorValidBits kValidAirPressure   = 1u << 0;
constexpr SensorValidBits kValidOilTempSafety = 1u << 1;
constexpr SensorValidBits kValidFloatMin      = 1u << 2;
constexpr SensorValidBits kValidFloatMax      = 1u << 3;

constexpr SensorValidBits kRequiredSensors =
    kValidAirPressure | kValidOilTempSafety | kValidFloatMin | kValidFloatMax;

// ТЭН: критичен минимум масла в подогреваемой ёмкости ("не на сухую").
constexpr SensorValidBits kHeaterRequiredSensors =
    kValidOilTempSafety | kValidFloatMin;

struct SafetyLease {
    bool            valid{false};
    bool            everValid{false};   // диагностика; владелец истории — SafetySupervisor
    bool            permit{false};
    bool            latched{false};
    GateState       gateState{GateState::Unknown};
    AlarmBits       alarmBits{0};
    SensorValidBits sensorValid{0};
    ContactorFb     contactorFb{ContactorFb::Unknown};
    PowerFb         kc868PowerFb{PowerFb::Unknown};
    uint32_t        rxStampMs{0};       // локальное время приёма
};

// Свежесть lease: valid + не старше 600 мс. Wrap-safe.
inline bool leaseFresh(const SafetyLease& lease, uint32_t nowMs) {
    return lease.valid &&
           static_cast<uint32_t>(nowMs - lease.rxStampMs) <= kLeaseTimeoutMs;
}

enum class SafetyVerdict : uint8_t { Allow = 0, Deny, Latch };

struct SafetyDecision {
    SafetyVerdict verdict{SafetyVerdict::Deny};
    bool          permitProcess{false};    // fan/prepurge
    bool          permitDangerous{false};  // fuel/spark/air
    bool          permitHeater{false};     // ТЭН: контактор+oil-temp, БЕЗ gate OPEN_SAFE
    bool          latchRequest{false};
    StopReason    reason{StopReason::PermitLost};
};

struct SafetyConfig {   // compile-time, НЕ из NVS
    bool requireContactorFb{false};
    bool requireKc868PowerFb{false};
};

// История, которой policy не может владеть (чистая функция получает её явно).
struct SafetyHistory {
    bool everValid{false};
    bool everPermitted{false};
    bool everContactorConfirmed{false};
    bool everPowerConfirmed{false};
};

constexpr const char* gateStateName(GateState state) {
    switch (state) {
        case GateState::Unknown: return "Unknown";
        case GateState::Closed:  return "Closed";
        case GateState::OpenSafe:return "OpenSafe";
        case GateState::Latched: return "Latched";
        case GateState::Fault:   return "Fault";
    }
    return "UNKNOWN";
}

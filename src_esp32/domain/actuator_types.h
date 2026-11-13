#pragma once
// Актуаторы и маски групп (§5.1). Host-clean.
// ACT_*_VALVE_SAFETY здесь отсутствуют by design: ими управляет только Nano.

#include <cstdint>
#include "domain/burner_types.h"

enum class ActuatorId : uint8_t {
    FanBlower = 0,
    OilValveMain,
    AirValveMain,
    IgnitionUnit,
    HeaterOil,
    PumpOilTransfer,
    PumpBoiler,
    Count
};

static_assert(static_cast<uint8_t>(ActuatorId::Count) <= 16,
              "ActuatorBits — 16 бит, актуаторов не больше 16");

using ActuatorBits = uint16_t;

constexpr ActuatorBits actuatorBit(ActuatorId id) {
    return ActuatorBits(1u) << static_cast<uint8_t>(id);
}

constexpr ActuatorBits kAllOff     = 0;
constexpr ActuatorBits kCombustion = actuatorBit(ActuatorId::OilValveMain) |
                                     actuatorBit(ActuatorId::AirValveMain);
constexpr ActuatorBits kIgnitionGroup = kCombustion | actuatorBit(ActuatorId::IgnitionUnit);
constexpr ActuatorBits kDangerGroup   = kIgnitionGroup | actuatorBit(ActuatorId::PumpOilTransfer);
constexpr ActuatorBits kSafetyOffGroup = kDangerGroup | actuatorBit(ActuatorId::HeaterOil);
constexpr ActuatorBits kProcessGroup  = actuatorBit(ActuatorId::FanBlower) |
                                        actuatorBit(ActuatorId::PumpBoiler);

constexpr ActuatorBits kFuelGroup   = actuatorBit(ActuatorId::OilValveMain) |
                                      actuatorBit(ActuatorId::PumpOilTransfer);
constexpr ActuatorBits kSparkGroup  = actuatorBit(ActuatorId::IgnitionUnit);
constexpr ActuatorBits kHeaterGroup = actuatorBit(ActuatorId::HeaterOil);
constexpr ActuatorBits kAirPath     = actuatorBit(ActuatorId::AirValveMain);

constexpr ActuatorBits kDefinedBits =
    (ActuatorBits(1u) << static_cast<uint8_t>(ActuatorId::Count)) - 1u;
constexpr ActuatorBits kManualAllowedMask = kDefinedBits;

static_assert((kCombustion & kIgnitionGroup) == kCombustion,
              "группа розжига должна включать горение");
static_assert((kDangerGroup & kFuelGroup) == kFuelGroup,
              "опасная группа должна включать топливо");
static_assert((kSafetyOffGroup & kHeaterGroup) == kHeaterGroup,
              "safety-off группа должна включать ТЭН");
static_assert((kManualAllowedMask & ~kDefinedBits) == 0,
              "manual-маска не может содержать неизвестных битов");

struct ActuatorDemand {
    ActuatorBits bits{kAllOff};
    BurnerState  state{BurnerState::Idle};
    bool         permitDangerous{false};   // fail-closed
    bool         permitHeater{false};      // ТЭН: отдельное разрешение
    bool         allowHeatOil{false};      // canHeatOil: ТЭН в Idle и Run
};

constexpr const char* actuatorName(ActuatorId id) {
    switch (id) {
        case ActuatorId::FanBlower:      return "fan";
        case ActuatorId::OilValveMain:   return "oil";
        case ActuatorId::AirValveMain:   return "air";
        case ActuatorId::IgnitionUnit:   return "ignition";
        case ActuatorId::HeaterOil:      return "heater";
        case ActuatorId::PumpOilTransfer:return "pump_transfer";
        case ActuatorId::PumpBoiler:     return "pump_boiler";
        case ActuatorId::Count:          return "count";
    }
    return "unknown";
}

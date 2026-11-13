#pragma once
// Разбор команд manual-консоли. Host-clean: без Arduino/Serial, без heap.
// IO (чтение строки, печать) делает composition root app_manual.cpp.

#include <cstdint>
#include "domain/actuator_types.h"

enum class ManualVerb : uint8_t {
    Invalid = 0,
    ManualOn,
    ManualOff,
    SetOne,
    AllOff,
    Keep,
    Sensors,
    Status
};

struct ManualCommand {
    ManualVerb  verb{ManualVerb::Invalid};
    ActuatorId  actuator{ActuatorId::FanBlower};
    bool        value{false};
};

// true — команда распознана. "set all 0" -> AllOff; "set all 1" — Invalid.
bool parseManualCommand(const char* line, ManualCommand& out);

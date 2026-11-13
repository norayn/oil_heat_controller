#include "manual/manual_console.h"

#include <cstring>

namespace {

constexpr uint8_t kMaxTokens = 3;
constexpr size_t  kTokenCap = 24;

bool tokenEq(const char* token, const char* literal) {
    return std::strcmp(token, literal) == 0;
}

bool actuatorFromToken(const char* token, ActuatorId& out) {
    if (tokenEq(token, "fan"))           { out = ActuatorId::FanBlower; return true; }
    if (tokenEq(token, "oil"))           { out = ActuatorId::OilValveMain; return true; }
    if (tokenEq(token, "air"))           { out = ActuatorId::AirValveMain; return true; }
    if (tokenEq(token, "ignition"))      { out = ActuatorId::IgnitionUnit; return true; }
    if (tokenEq(token, "heater"))        { out = ActuatorId::HeaterOil; return true; }
    if (tokenEq(token, "pump_transfer")) { out = ActuatorId::PumpOilTransfer; return true; }
    if (tokenEq(token, "pump_boiler"))   { out = ActuatorId::PumpBoiler; return true; }
    return false;
}

uint8_t tokenize(const char* line, char tokens[kMaxTokens][kTokenCap]) {
    uint8_t count = 0;
    const char* p = line;

    while (*p != '\0' && count < kMaxTokens) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        size_t len = 0;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            if (len + 1 < kTokenCap) {
                tokens[count][len] = *p;
                ++len;
            }
            ++p;
        }
        tokens[count][len] = '\0';
        ++count;
    }

    return count;
}

}  // namespace

bool parseManualCommand(const char* line, ManualCommand& out) {
    if (line == nullptr) {
        return false;
    }

    char tokens[kMaxTokens][kTokenCap] = {};
    const uint8_t count = tokenize(line, tokens);
    out = ManualCommand{};

    if (count == 1) {
        if (tokenEq(tokens[0], "keep")) {
            out.verb = ManualVerb::Keep;
            return true;
        }
        if (tokenEq(tokens[0], "sensors")) {
            out.verb = ManualVerb::Sensors;
            return true;
        }
        if (tokenEq(tokens[0], "status")) {
            out.verb = ManualVerb::Status;
            return true;
        }
        return false;
    }

    if (count == 2) {
        if (!tokenEq(tokens[0], "manual")) {
            return false;
        }
        if (tokenEq(tokens[1], "on")) {
            out.verb = ManualVerb::ManualOn;
            return true;
        }
        if (tokenEq(tokens[1], "off")) {
            out.verb = ManualVerb::ManualOff;
            return true;
        }
        return false;
    }

    if (count == 3) {
        if (!tokenEq(tokens[0], "set")) {
            return false;
        }

        if (tokenEq(tokens[1], "all")) {
            if (tokenEq(tokens[2], "0")) {
                out.verb = ManualVerb::AllOff;
                return true;
            }
            return false;
        }

        if (!actuatorFromToken(tokens[1], out.actuator)) {
            return false;
        }
        if (tokenEq(tokens[2], "1")) {
            out.verb = ManualVerb::SetOne;
            out.value = true;
            return true;
        }
        if (tokenEq(tokens[2], "0")) {
            out.verb = ManualVerb::SetOne;
            out.value = false;
            return true;
        }
        return false;
    }

    return false;
}

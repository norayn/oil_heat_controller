#include "platform/log.h"
#include <Arduino.h>
#include <cstdarg>

namespace {

const char* levelName(platform::LogLevel level) {
    switch (level) {
        case platform::LogLevel::Error: return "E";
        case platform::LogLevel::Warn:  return "W";
        case platform::LogLevel::Info:  return "I";
        case platform::LogLevel::Debug: return "D";
    }
    return "?";
}

}  // namespace

namespace platform {

void logBegin() {
    Serial.begin(115200);
}

void logf(LogLevel level, const char* tag, const char* format, ...) {
    char buffer[192];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[%s][%s] %s\n", levelName(level), tag, buffer);
}

void logRaw(const char* text) {
    Serial.println(text);
}

}  // namespace platform

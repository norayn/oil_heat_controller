#include "platform/log.h"
#include <cstdarg>
#include <cstdio>

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

void logBegin() {}

void logf(LogLevel level, const char* tag, const char* format, ...) {
    std::printf("[%s][%s] ", levelName(level), tag);

    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);

    std::printf("\n");
    std::fflush(stdout);
}

void logRaw(const char* text) {
    std::printf("%s\n", text);
    std::fflush(stdout);
}

}  // namespace platform

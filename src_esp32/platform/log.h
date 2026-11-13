#pragma once
// Логи с уровнями и тегами. ESP32 — Serial, host — stdout.
// Rate limit и ring buffer появятся в M1 (Diagnostics).

#include <cstdint>

namespace platform {

enum class LogLevel : uint8_t { Error = 0, Warn, Info, Debug };

void logBegin();
void logf(LogLevel level, const char* tag, const char* format, ...);
void logRaw(const char* text);

}  // namespace platform

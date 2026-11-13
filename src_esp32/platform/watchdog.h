#pragma once
// Task watchdog. ESP32 — TWDT; host-сборки этот файл не компилируют.

#include <cstdint>

namespace platform {

void watchdogBegin(uint32_t timeoutMs);
void watchdogFeed();

}  // namespace platform

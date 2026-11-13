#pragma once
// Глобальное монотонное время. ESP32 — Arduino millis(), host — управляемые часы.
// В domain/services время передаётся параметром nowMs; этот заголовок нужен
// только composition root и платформенным адаптерам.

#include <cstdint>

namespace platform {

uint32_t nowMs();

}  // namespace platform

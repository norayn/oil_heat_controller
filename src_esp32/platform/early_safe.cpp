#include "platform/early_safe.h"
#include <Arduino.h>

namespace platform {

void earlyApplicationSafe() {
    // Пины реле/SSR ещё не зафиксированы: docs/hardware/pinmap.md — DRAFT.
    // Здесь ДО Serial/I2C должны выставляться direct GPIO в OFF/pull-down.
    // M1: добавить конкретные линии из pinmap и проверить на плате.
    //
    // PCF8574 здесь не трогаем: до записи 0x00 его состояние не контролируется
    // (POR может активировать реле, §11.1). Это ограничение схемы, не кода.
}

}  // namespace platform

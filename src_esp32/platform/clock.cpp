#include "platform/clock.h"
#include <Arduino.h>

namespace platform {

uint32_t nowMs() {
    return static_cast<uint32_t>(millis());
}

}  // namespace platform

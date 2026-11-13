#include "platform/watchdog.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

namespace platform {

void watchdogBegin(uint32_t timeoutMs) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t config{};
    config.timeout_ms = timeoutMs;
    config.idle_core_mask = 0;
    config.trigger_panic = true;
    (void)esp_task_wdt_init(&config);
#else
    const uint32_t seconds = timeoutMs / 1000u == 0 ? 1u : timeoutMs / 1000u;
    (void)esp_task_wdt_init(seconds, true);
#endif

    // Arduino loopTask может быть уже подписан; повторная подписка безвредна.
    (void)esp_task_wdt_add(nullptr);
}

void watchdogFeed() {
    (void)esp_task_wdt_reset();
}

}  // namespace platform

#include "app/app.h"

#include "config/limits.h"
#include "platform/clock.h"
#include "platform/early_safe.h"
#include "platform/log.h"
#include "platform/watchdog.h"
#include "services/actuator_service.h"
#include "services/scheduler.h"

namespace {

ActuatorService g_actuators;
Scheduler        g_scheduler;
PeriodicTimer    g_controlTimer;

}  // namespace

namespace app {

ActuatorService& actuators() {
    return g_actuators;
}

Scheduler& scheduler() {
    return g_scheduler;
}

SafetyConfig safetyConfig() {
    SafetyConfig cfg;
    cfg.requireContactorFb = kRequireContactorFb;
    cfg.requireKc868PowerFb = kRequireKc868PowerFb;
    return cfg;
}

void setup() {
    // 1. Direct GPIO OFF/pull-down до Serial/I2C (§11.1).
    platform::earlyApplicationSafe();

    // 2. Лог/баннер, затем watchdog.
    platform::logBegin();
    platform::logf(platform::LogLevel::Info, "APP", "profile: %s", profileName());
    platform::watchdogBegin(kWdtTimeoutMs);

    const uint32_t now = platform::nowMs();

    // 3. Composition root связывает порты, затем единственный писатель
    //    переводит выходы в OFF.
    setupProfile();
    g_actuators.beginSafe();

    // 4. Планировщик.
    g_scheduler.begin(now);
    g_controlTimer.start(now, kControlPeriodMs);

    platform::logf(platform::LogLevel::Info, "APP",
                   "boot: outputsUnknown=%d control=%u ms",
                   g_actuators.outputsUnknown() ? 1 : 0,
                   static_cast<unsigned>(kControlPeriodMs));
}

void tick() {
    const uint32_t now = platform::nowMs();

    if (g_controlTimer.due(now)) {
        tickProfile();
    }

    // Кормим watchdog в конце; при зависании профиля до сюда не доходим.
    platform::watchdogFeed();
}

}  // namespace app

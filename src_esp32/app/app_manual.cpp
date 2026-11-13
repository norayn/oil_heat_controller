// MANUAL composition root (esp32_manual).
// SafetySupervisor/SafetyLink/FSM/interlocks/retry не создаются.
// Управление — прямыми командами по одному актуатору + max-on/min-off/deadman.

#include <Arduino.h>

#include "app/app.h"
#include "config/limits.h"
#include "domain/mode_manager.h"
#include "manual/manual_console.h"
#include "manual/manual_control.h"
#include "manual/manual_monitor.h"
#include "platform/clock.h"
#include "platform/log.h"
#include "services/actuator_service.h"

namespace {

ManualControlService g_manual;
ManualMonitor        g_monitor;
ModeManager          g_mode;

char    g_line[64];
uint8_t g_lineLen = 0;

void printStatus(uint32_t nowMs) {
    (void)nowMs;
    ActuatorService& out = app::actuators();
    platform::logf(platform::LogLevel::Info, "STATUS",
                   "mode=%s manual=%s applied=0x%04x outputsUnknown=%d",
                   runModeName(g_mode.mode()),
                   g_manual.enabled() ? "ON" : "OFF",
                   static_cast<unsigned>(out.applied()),
                   out.outputsUnknown() ? 1 : 0);

    for (uint8_t i = 0; i < static_cast<uint8_t>(ActuatorId::Count); ++i) {
        const ActuatorId id = static_cast<ActuatorId>(i);
        platform::logf(platform::LogLevel::Info, "STATUS",
                       "  %s: applied=%d readback=%d maxOn=%u ms",
                       actuatorName(id),
                       (out.applied() & actuatorBit(id)) != 0 ? 1 : 0,
                       static_cast<int>(out.readback(id)),
                       static_cast<unsigned>(ManualControlService::maxOnMs(id)));
    }
}

void handleCommand(const ManualCommand& cmd, uint32_t nowMs) {
    ActuatorService& out = app::actuators();

    switch (cmd.verb) {
        case ManualVerb::ManualOn:
            if (g_manual.enter(nowMs)) {
                g_mode.requestManual(nowMs, true);
            }
            break;

        case ManualVerb::ManualOff:
            g_manual.exit(nowMs);
            g_mode.leaveManual(nowMs);
            break;

        case ManualVerb::AllOff:
            g_manual.allOffManual(nowMs);
            g_mode.leaveManual(nowMs);
            break;

        case ManualVerb::SetOne:
            if (!g_manual.setManual(cmd.actuator, cmd.value, nowMs)) {
                platform::logf(platform::LogLevel::Warn, "MANUAL",
                               "команда отклонена: %s=%d",
                               actuatorName(cmd.actuator), cmd.value ? 1 : 0);
            }
            break;

        case ManualVerb::Keep:
            g_manual.keep(nowMs);
            break;

        case ManualVerb::Sensors:
            platform::logf(platform::LogLevel::Warn, "SENSORS",
                           "SensorService появится в M2; выходы: applied=0x%04x",
                           static_cast<unsigned>(out.applied()));
            break;

        case ManualVerb::Status:
            printStatus(nowMs);
            break;

        case ManualVerb::Invalid:
        default:
            platform::logf(platform::LogLevel::Warn, "CONSOLE",
                           "неизвестная команда");
            break;
    }
}

void pollConsole(uint32_t nowMs) {
    while (Serial.available() > 0) {
        const int ch = Serial.read();
        if (ch < 0) {
            break;
        }

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            g_line[g_lineLen] = '\0';
            if (g_lineLen > 0) {
                ManualCommand cmd;
                if (parseManualCommand(g_line, cmd)) {
                    handleCommand(cmd, nowMs);
                } else {
                    platform::logf(platform::LogLevel::Warn, "CONSOLE",
                                   "неизвестная команда: %s", g_line);
                }
            }
            g_lineLen = 0;
            continue;
        }

        if (g_lineLen + 1 < sizeof(g_line)) {
            g_line[g_lineLen++] = static_cast<char>(ch);
        } else {
            g_lineLen = 0;   // переполнение: строку отбрасываем
        }
    }
}

}  // namespace

namespace app {

const char* profileName() {
    return "MANUAL BYPASS - SAFETY SUPERVISOR DISABLED";
}

void setupProfile() {
    const uint32_t now = platform::nowMs();

    g_mode.begin(now);
    g_mode.finishSelfCheck(now, true);

    g_manual.begin(&app::actuators(), now);
    g_monitor.begin(now);

    platform::logf(platform::LogLevel::Warn, "MANUAL",
                   "manual on | manual off | set <fan|oil|air|ignition|heater|"
                   "pump_transfer|pump_boiler> <0|1> | keep | status | sensors");
}

void tickProfile() {
    const uint32_t now = platform::nowMs();

    pollConsole(now);
    g_manual.tick(now);
    g_monitor.tick(now, app::actuators().applied(),
                   app::actuators().outputsUnknown());
}

}  // namespace app

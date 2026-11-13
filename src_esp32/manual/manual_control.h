#pragma once
// Ручное управление: вход/выход, per-actuator команды, max-on/min-off/deadman.
// Единственный runtime-limiter manual (§8.1). Host-clean: тестируется в native_manual.

#include <cstdint>
#include "domain/actuator_types.h"
#include "manual/manual_actuator_port.h"

class ActuatorService;

class ManualControlService final : public IManualActuatorPort {
public:
    void begin(ActuatorService* actuators, uint32_t nowMs);

    bool enter(uint32_t nowMs);
    void exit(uint32_t nowMs);
    bool enabled() const { return enabled_; }

    bool setManual(ActuatorId id, bool on, uint32_t nowMs) override;
    void allOffManual(uint32_t nowMs) override;

    // Продлевает deadman только для включённых актуаторов опасной группы.
    void keep(uint32_t nowMs);

    // Периодическая проверка лимитов + единственная запись в железо.
    void tick(uint32_t nowMs);

    ActuatorBits bits() const { return bits_; }

    static uint32_t maxOnMs(ActuatorId id);
    static uint32_t minOffMs(ActuatorId id);
    static uint32_t deadmanMs(ActuatorId id);

private:
    struct Runtime {
        bool     on{false};
        bool     offStampValid{false};
        uint32_t onSinceMs{0};
        uint32_t lastKeepMs{0};
        uint32_t offSinceMs{0};
    };

    void trip(ActuatorId id, uint32_t nowMs, const char* reason);
    void clearAll(uint32_t nowMs);

    ActuatorService* actuators_{nullptr};
    Runtime          rt_[static_cast<uint8_t>(ActuatorId::Count)]{};
    ActuatorBits     bits_{kAllOff};
    bool             enabled_{false};
};

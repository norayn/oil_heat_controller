#pragma once
// Порт прямого управления актуаторами. ТОЛЬКО manual-сборка.
// Реализация — ManualControlService: единственный владелец вектора.

#include <cstdint>
#include "domain/actuator_types.h"

class IManualActuatorPort {
public:
    virtual ~IManualActuatorPort() = default;

    // По одному актуатору. Валидирует и обновляет вектор владельца,
    // но не пишет в железо: запись — только в ActuatorService.
    virtual bool setManual(ActuatorId id, bool on, uint32_t nowMs) = 0;
    virtual void allOffManual(uint32_t nowMs) = 0;
};

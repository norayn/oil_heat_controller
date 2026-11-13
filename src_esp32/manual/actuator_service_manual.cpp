#pragma once
// ЕДИНСТВЕННОЕ определение ActuatorService::tickManual.
// Файл входит только в manual-сборку; в FIELD символа tickManual нет.

#include "services/actuator_service.h"

void ActuatorService::tickManual(uint32_t nowMs, ActuatorBits bits) {
    // Неизвестный бит — отказ всей команды, а не «игнорировать».
    if ((bits & ~kDefinedBits) != 0) {
        return;
    }

    // I2C fault: фактическое состояние неизвестно, опасную группу не включаем.
    if (outputsUnknown_) {
        bits &= ~kSafetyOffGroup;
    }

    apply(nowMs, bits);
}

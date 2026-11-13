#pragma once
// Единственный писатель актуаторов (ADR-04, §8.1).
//
// tickAuto — FIELD: применяет инвариант §5.1 по demand.state и обе маски.
// tickManual — manual: объявлен всегда, ОПРЕДЕЛЁН только в
//   manual/actuator_service_manual.cpp. В FIELD символа нет: случайный вызов —
//   ошибка линковки, а не runtime-обход.

#include <cstdint>
#include "domain/actuator_types.h"
#include "hal/ports.h"

class ActuatorService {
public:
    static constexpr uint8_t kActuatorCount =
        static_cast<uint8_t>(ActuatorId::Count);

    void bind(ActuatorId id, IDigitalOut* out);
    void beginSafe();

    void tickAuto(uint32_t nowMs, const ActuatorDemand& demand);
    void tickManual(uint32_t nowMs, ActuatorBits bits);

    ActuatorBits applied() const { return applied_; }
    Readback readback(ActuatorId id) const;
    bool readbackMismatch() const;
    bool outputsUnknown() const { return outputsUnknown_; }
    void setOutputsUnknown(bool unknown) { outputsUnknown_ = unknown; }

    // Диагностика нарушения инварианта §5.1 в последнем tickAuto.
    bool lastDemandViolated() const { return lastDemandViolated_; }

    // Чистая проверка инварианта §5.1: возвращает допустимый набор битов.
    // violated=true — demand нарушил инвариант состояния (лишний бит снят).
    static ActuatorBits applyInvariant(const ActuatorDemand& demand, bool& violated);

private:
    void apply(uint32_t nowMs, ActuatorBits bits);

    IDigitalOut* outs_[kActuatorCount]{};
    ActuatorBits applied_{kAllOff};
    bool         outputsUnknown_{false};
    bool         lastDemandViolated_{false};
};

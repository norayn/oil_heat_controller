#include "services/actuator_service.h"

void ActuatorService::bind(ActuatorId id, IDigitalOut* out) {
    const uint8_t index = static_cast<uint8_t>(id);
    if (index < kActuatorCount) {
        outs_[index] = out;
    }
}

void ActuatorService::beginSafe() {
    outputsUnknown_ = false;
    applied_ = kAllOff;

    for (uint8_t i = 0; i < kActuatorCount; ++i) {
        if (outs_[i] == nullptr) {
            outputsUnknown_ = true;
            continue;
        }
        outs_[i]->write(false);
        const Readback rb = outs_[i]->readback();
        // Подтверждённое OFF — только Off. On/Unknown/NotAvailable означают,
        // что фактическое состояние неизвестно.
        if (rb != Readback::Off) {
            outputsUnknown_ = true;
        }
    }
}

// Инвариант §5.1. Лишние биты снимаются; неизвестный бит — отказ всей команды.
// Маски !permitDangerous/!permitHeater — штатное fail-closed поведение,
// а не нарушение состояния, поэтому violated по ним не поднимается.
ActuatorBits ActuatorService::applyInvariant(const ActuatorDemand& demand,
                                             bool& violated) {
    violated = false;

    ActuatorBits out = demand.bits;
    if ((out & ~kDefinedBits) != 0) {
        violated = true;
        return kAllOff;
    }

    switch (demand.state) {
        case BurnerState::Init:
            if (out != kAllOff) {
                violated = true;
            }
            out = kAllOff;
            break;

        case BurnerState::Latched:
        case BurnerState::Shutdown:
        case BurnerState::Fault:
        case BurnerState::PostPurge:
        case BurnerState::PrePurge: {
            const ActuatorBits bad = out & kSafetyOffGroup;
            if (bad != 0) {
                violated = true;
                out &= ~kSafetyOffGroup;
            }
            break;
        }

        case BurnerState::Idle: {
            const ActuatorBits bad = out & (kFuelGroup | kSparkGroup | kAirPath);
            if (bad != 0) {
                violated = true;
                out &= ~(kFuelGroup | kSparkGroup | kAirPath);
            }
            break;
        }

        case BurnerState::Ignition: {
            const ActuatorBits bad = out & (kHeaterGroup |
                                            actuatorBit(ActuatorId::PumpOilTransfer));
            if (bad != 0) {
                violated = true;
                out &= ~bad;
            }
            break;
        }

        case BurnerState::Run: {
            const ActuatorBits required =
                actuatorBit(ActuatorId::FanBlower) | kAirPath;
            if ((out & required) != required) {
                // Без вентилятора и рабочего воздуха топливо не подаётся.
                violated = true;
                out &= ~(kFuelGroup | kSparkGroup);
            }

            if ((out & kSparkGroup) != 0) {
                violated = true;
                out &= ~kSparkGroup;
            }

            const ActuatorBits extraFuel =
                out & kFuelGroup & ~actuatorBit(ActuatorId::OilValveMain);
            if (extraFuel != 0) {
                violated = true;
                out &= ~extraFuel;
            }
            break;
        }
    }

    // ТЭН: только при permitHeater && allowHeatOil (Idle и Run).
    // В остальных состояниях его уже сняли выше.
    if (demand.state == BurnerState::Idle || demand.state == BurnerState::Run) {
        if (!(demand.permitHeater && demand.allowHeatOil)) {
            out &= ~kHeaterGroup;
        }
    }

    // Разрешения — всегда fail-closed.
    if (!demand.permitDangerous) {
        out &= ~kDangerGroup;
    }
    if (!demand.permitHeater) {
        out &= ~kHeaterGroup;
    }

    return out;
}

void ActuatorService::tickAuto(uint32_t nowMs, const ActuatorDemand& demand) {
    bool violated = false;
    ActuatorBits out = applyInvariant(demand, violated);
    lastDemandViolated_ = violated;

    // I2C fault: фактическое состояние неизвестно — опасная группа и ТЭН OFF.
    if (outputsUnknown_) {
        out &= ~kSafetyOffGroup;
    }

    apply(nowMs, out);
}

void ActuatorService::apply(uint32_t nowMs, ActuatorBits bits) {
    (void)nowMs;

    for (uint8_t i = 0; i < kActuatorCount; ++i) {
        const ActuatorBits mask = ActuatorBits(1u) << i;
        if (outs_[i] == nullptr) {
            if ((bits & mask) != 0) {
                outputsUnknown_ = true;
            }
            continue;
        }

        const bool want = (bits & mask) != 0;
        const bool have = (applied_ & mask) != 0;
        if (want != have) {
            outs_[i]->write(want);
        }
    }

    applied_ = bits;
}

Readback ActuatorService::readback(ActuatorId id) const {
    const uint8_t index = static_cast<uint8_t>(id);
    if (index >= kActuatorCount || outs_[index] == nullptr) {
        return Readback::NotAvailable;
    }
    return outs_[index]->readback();
}

bool ActuatorService::readbackMismatch() const {
    for (uint8_t i = 0; i < kActuatorCount; ++i) {
        if (outs_[i] == nullptr) {
            continue;
        }

        const bool want = (applied_ & (ActuatorBits(1u) << i)) != 0;
        const Readback rb = outs_[i]->readback();
        if (rb == Readback::On && !want) {
            return true;
        }
        if (rb == Readback::Off && want) {
            return true;
        }
    }
    return false;
}

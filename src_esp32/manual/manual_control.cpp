#include "manual/manual_control.h"

#include "config/limits.h"
#include "platform/log.h"
#include "services/actuator_service.h"

namespace {

constexpr uint8_t kCount = static_cast<uint8_t>(ActuatorId::Count);

uint8_t indexOf(ActuatorId id) {
    return static_cast<uint8_t>(id);
}

}  // namespace

uint32_t ManualControlService::maxOnMs(ActuatorId id) {
    switch (id) {
        case ActuatorId::FanBlower:       return kManualMaxOnFanMs;
        case ActuatorId::OilValveMain:    return kManualMaxOnValveMs;
        case ActuatorId::AirValveMain:    return kManualMaxOnValveMs;
        case ActuatorId::IgnitionUnit:    return kManualMaxOnIgnitionMs;
        case ActuatorId::HeaterOil:       return kManualMaxOnHeaterMs;
        case ActuatorId::PumpOilTransfer: return kManualMaxOnPumpTransferMs;
        case ActuatorId::PumpBoiler:      return kManualMaxOnPumpBoilerMs;
        case ActuatorId::Count:           return 0;
    }
    return 0;
}

uint32_t ManualControlService::minOffMs(ActuatorId id) {
    switch (id) {
        case ActuatorId::FanBlower:       return kManualMinOffFanMs;
        case ActuatorId::OilValveMain:    return kManualMinOffValveMs;
        case ActuatorId::AirValveMain:    return kManualMinOffValveMs;
        case ActuatorId::IgnitionUnit:    return kManualMinOffIgnitionMs;
        case ActuatorId::HeaterOil:       return kManualMinOffHeaterMs;
        case ActuatorId::PumpOilTransfer: return kManualMinOffPumpTransferMs;
        case ActuatorId::PumpBoiler:      return kManualMinOffPumpBoilerMs;
        case ActuatorId::Count:           return 0;
    }
    return 0;
}

uint32_t ManualControlService::deadmanMs(ActuatorId id) {
    switch (id) {
        case ActuatorId::OilValveMain:
        case ActuatorId::AirValveMain:
        case ActuatorId::IgnitionUnit:
        case ActuatorId::HeaterOil:
        case ActuatorId::PumpOilTransfer:
            return kManualDeadmanMs;
        case ActuatorId::FanBlower:
        case ActuatorId::PumpBoiler:
        case ActuatorId::Count:
            return 0;
    }
    return 0;
}

void ManualControlService::begin(ActuatorService* actuators, uint32_t nowMs) {
    actuators_ = actuators;
    enabled_ = false;
    bits_ = kAllOff;
    for (uint8_t i = 0; i < kCount; ++i) {
        rt_[i] = Runtime{};
        rt_[i].offSinceMs = nowMs;
    }
}

bool ManualControlService::enter(uint32_t nowMs) {
    if (actuators_ == nullptr || enabled_) {
        return false;
    }
    if (actuators_->outputsUnknown() || actuators_->readbackMismatch()) {
        platform::logf(platform::LogLevel::Warn, "MANUAL",
                       "вход запрещён: состояние выходов неизвестно");
        return false;
    }

    // «Все выходы OFF до команд» — проверяем фактический read-back.
    for (uint8_t i = 0; i < kCount; ++i) {
        if (actuators_->readback(static_cast<ActuatorId>(i)) != Readback::Off) {
            platform::logf(platform::LogLevel::Warn, "MANUAL",
                           "вход запрещён: %s не подтверждён OFF",
                           actuatorName(static_cast<ActuatorId>(i)));
            return false;
        }
    }

    bits_ = kAllOff;
    for (uint8_t i = 0; i < kCount; ++i) {
        rt_[i] = Runtime{};
        rt_[i].offSinceMs = nowMs;
        rt_[i].offStampValid = false;
    }
    enabled_ = true;
    platform::logf(platform::LogLevel::Warn, "MANUAL", "Manual ON");
    return true;
}

void ManualControlService::clearAll(uint32_t nowMs) {
    for (uint8_t i = 0; i < kCount; ++i) {
        rt_[i].on = false;
        rt_[i].offStampValid = false;
        rt_[i].offSinceMs = nowMs;
    }
    bits_ = kAllOff;
}

void ManualControlService::exit(uint32_t nowMs) {
    if (enabled_) {
        platform::logf(platform::LogLevel::Warn, "MANUAL", "Manual OFF");
    }
    enabled_ = false;
    clearAll(nowMs);
    if (actuators_ != nullptr) {
        actuators_->tickManual(nowMs, kAllOff);
    }
}

void ManualControlService::allOffManual(uint32_t nowMs) {
    exit(nowMs);
}

bool ManualControlService::setManual(ActuatorId id, bool on, uint32_t nowMs) {
    if (!enabled_ || actuators_ == nullptr) {
        return false;
    }

    const uint8_t index = indexOf(id);
    if (index >= kCount) {
        return false;
    }
    const ActuatorBits mask = actuatorBit(id);
    if ((mask & kManualAllowedMask) == 0) {
        return false;
    }

    if (actuators_->outputsUnknown()) {
        platform::logf(platform::LogLevel::Error, "MANUAL",
                       "I2C fault: команды отклонены, выход из Manual");
        exit(nowMs);
        return false;
    }

    Runtime& r = rt_[index];

    if (on) {
        if (r.on) {
            // Повторный set x 1 окно не сбрасывает — продлевает только keep.
            return true;
        }
        if (r.offStampValid) {
            const uint32_t offFor = nowMs - r.offSinceMs;
            if (offFor < minOffMs(id)) {
                platform::logf(platform::LogLevel::Warn, "MANUAL",
                               "%s: min-off ещё не истёк", actuatorName(id));
                return false;
            }
        }
        r.on = true;
        r.offStampValid = false;
        r.onSinceMs = nowMs;
        r.lastKeepMs = nowMs;
        bits_ |= mask;
        platform::logf(platform::LogLevel::Info, "MANUAL", "%s=1",
                       actuatorName(id));
        return true;
    }

    if (r.on) {
        r.on = false;
        r.offStampValid = true;
        r.offSinceMs = nowMs;
        bits_ &= ~mask;
        platform::logf(platform::LogLevel::Info, "MANUAL", "%s=0",
                       actuatorName(id));
    }
    return true;
}

void ManualControlService::keep(uint32_t nowMs) {
    if (!enabled_) {
        return;
    }
    for (uint8_t i = 0; i < kCount; ++i) {
        const ActuatorId id = static_cast<ActuatorId>(i);
        if (rt_[i].on && deadmanMs(id) > 0) {
            rt_[i].lastKeepMs = nowMs;
        }
    }
}

void ManualControlService::trip(ActuatorId id, uint32_t nowMs, const char* reason) {
    const uint8_t index = indexOf(id);
    rt_[index].on = false;
    rt_[index].offStampValid = true;
    rt_[index].offSinceMs = nowMs;
    bits_ &= ~actuatorBit(id);
    platform::logf(platform::LogLevel::Warn, "MANUAL", "%s OFF: %s",
                   actuatorName(id), reason);
}

void ManualControlService::tick(uint32_t nowMs) {
    if (actuators_ == nullptr) {
        return;
    }

    if (!enabled_) {
        actuators_->tickManual(nowMs, kAllOff);
        return;
    }

    if (actuators_->outputsUnknown()) {
        platform::logf(platform::LogLevel::Error, "MANUAL",
                       "I2C fault: allOff, выход из Manual; "
                       "ОТКЛЮЧИТЕ ПИТАНИЕ НАГРУЗОК");
        exit(nowMs);
        return;
    }

    for (uint8_t i = 0; i < kCount; ++i) {
        const ActuatorId id = static_cast<ActuatorId>(i);
        if (!rt_[i].on) {
            continue;
        }

        const uint32_t maxOn = maxOnMs(id);
        if (maxOn > 0 && (nowMs - rt_[i].onSinceMs) >= maxOn) {
            trip(id, nowMs, "max-on");
            continue;
        }

        const uint32_t deadman = deadmanMs(id);
        if (deadman > 0 && (nowMs - rt_[i].lastKeepMs) >= deadman) {
            trip(id, nowMs, "deadman");
        }
    }

    actuators_->tickManual(nowMs, bits_);
}

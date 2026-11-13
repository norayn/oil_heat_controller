#include "domain/safety_policy.h"

namespace {

SafetyDecision denyDecision() {
    SafetyDecision d;   // fail-closed: Deny, все permit=false, reason=PermitLost
    return d;
}

SafetyDecision latchDecision(StopReason reason) {
    SafetyDecision d;
    d.verdict = SafetyVerdict::Latch;
    d.latchRequest = true;
    d.reason = reason;
    return d;
}

SafetyDecision allowDecision() {
    SafetyDecision d;
    d.verdict = SafetyVerdict::Allow;
    d.permitProcess = true;
    d.reason = StopReason::None;
    return d;
}

}  // namespace

// Таблица решений §7.3. Порядок проверок — сверху вниз; первое совпадение
// определяет verdict. verdict относится только к process; опасную группу
// разрешает исключительно permitDangerous, ТЭН — permitHeater.
SafetyDecision safetyPolicyEvaluate(const SafetyLease& lease,
                                    const SafetyHistory& history,
                                    const FaultBits& localFaults,
                                    uint32_t nowMs,
                                    const SafetyConfig& cfg) {
    // Локальные CRITICAL/HIGH: липкий latch независимо от lease.
    if (localFaults & kLatchFaultMask) {
        return latchDecision(stopReasonFromFaults(localFaults));
    }

    const bool fresh = leaseFresh(lease, nowMs);

    // Нет валидного/свежего lease.
    if (!fresh) {
        if (history.everValid) {
            return latchDecision(StopReason::LinkLoss);
        }
        return denyDecision();
    }

    // Nano уже латчился или выставил любой alarm-бит (включая неизвестный).
    if (lease.latched) {
        return latchDecision(StopReason::SafetyAlarm);
    }
    if (lease.alarmBits != 0) {
        return latchDecision(StopReason::SafetyAlarm);
    }

    // Технологическое разрешение.
    if (!lease.permit) {
        if (history.everPermitted) {
            return latchDecision(StopReason::PermitLost);
        }
        return denyDecision();
    }

    // Gate: FAULT/UNKNOWN — недостоверно, LATCHED — Nano уже латчился.
    if (lease.gateState == GateState::Fault ||
        lease.gateState == GateState::Unknown) {
        return latchDecision(StopReason::GateFault);
    }
    if (lease.gateState == GateState::Latched) {
        return latchDecision(StopReason::SafetyAlarm);
    }

    // Обратная связь питания платы (KC868_12V_FB).
    if (cfg.requireKc868PowerFb) {
        if (lease.kc868PowerFb == PowerFb::Fault) {
            return latchDecision(StopReason::SafetyAlarm);
        }
        if (lease.kc868PowerFb != PowerFb::Present &&
            history.everPowerConfirmed) {
            return latchDecision(StopReason::SafetyAlarm);
        }
    }

    // Обратная связь общего контактора (BLK-002).
    if (cfg.requireContactorFb) {
        if (lease.contactorFb == ContactorFb::Fault) {
            return latchDecision(StopReason::SafetyAlarm);
        }
        if (lease.contactorFb != ContactorFb::OnConfirmed &&
            history.everContactorConfirmed) {
            return latchDecision(StopReason::SafetyAlarm);
        }
    }

    // Процесс разрешён. Дальше — только отдельные разрешения.
    SafetyDecision d = allowDecision();

    const bool sensorsOk =
        (lease.sensorValid & kRequiredSensors) == kRequiredSensors;
    const bool heaterSensorsOk =
        (lease.sensorValid & kHeaterRequiredSensors) == kHeaterRequiredSensors;
    const bool gateSafe = lease.gateState == GateState::Closed ||
                          lease.gateState == GateState::OpenSafe;
    const bool contactorOk = !cfg.requireContactorFb ||
                             lease.contactorFb == ContactorFb::OnConfirmed;
    const bool powerOk = !cfg.requireKc868PowerFb ||
                         lease.kc868PowerFb == PowerFb::Present;

    // ТЭН не требует OPEN_SAFE: он не подаёт топливо и не участвует в розжиге.
    // Но требует минимум масла, валидную температуру, контактор и питание.
    d.permitHeater = gateSafe && contactorOk && powerOk && heaterSensorsOk;

    // Опасная группа: полный набор датчиков + gate OPEN_SAFE + обе обратные связи.
    d.permitDangerous = sensorsOk &&
                        lease.gateState == GateState::OpenSafe &&
                        powerOk &&
                        contactorOk;

    return d;
}

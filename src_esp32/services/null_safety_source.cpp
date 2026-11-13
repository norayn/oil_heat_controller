#include "services/null_safety_source.h"

void NullSafetySource::poll(uint32_t nowMs) {
    (void)nowMs;
    // Валидного кадра нет: valid=false, permit=false.
    lease_.valid = false;
    lease_.permit = false;
    lease_.latched = false;
    lease_.gateState = GateState::Unknown;
    lease_.alarmBits = 0;
    lease_.sensorValid = 0;
    lease_.contactorFb = ContactorFb::Unknown;
    lease_.kc868PowerFb = PowerFb::Unknown;
    lease_.rxStampMs = 0;
}

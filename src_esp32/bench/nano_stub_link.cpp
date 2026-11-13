#include "bench/nano_stub_link.h"

void NanoStubLink::poll(uint32_t nowMs) {
    lease_.valid = true;
    lease_.everValid = true;
    lease_.permit = true;
    lease_.latched = false;
    lease_.gateState = GateState::OpenSafe;
    lease_.alarmBits = 0;
    lease_.sensorValid = kRequiredSensors;
    lease_.contactorFb = ContactorFb::OnConfirmed;
    lease_.kc868PowerFb = PowerFb::Present;
    lease_.rxStampMs = nowMs;
}

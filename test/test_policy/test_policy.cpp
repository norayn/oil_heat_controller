#include <unity.h>

#include "domain/safety_policy.h"
#include "domain/safety_supervisor.h"

namespace {

constexpr uint32_t kNow = 100000;

SafetyLease fullLease() {
    SafetyLease lease;
    lease.valid = true;
    lease.permit = true;
    lease.latched = false;
    lease.gateState = GateState::OpenSafe;
    lease.alarmBits = 0;
    lease.sensorValid = kRequiredSensors;
    lease.contactorFb = ContactorFb::OnConfirmed;
    lease.kc868PowerFb = PowerFb::Present;
    lease.rxStampMs = kNow;
    return lease;
}

SafetyConfig fullConfig() {
    SafetyConfig cfg;
    cfg.requireContactorFb = true;
    cfg.requireKc868PowerFb = true;
    return cfg;
}

void assertNoPermits(const SafetyDecision& d) {
    TEST_ASSERT_FALSE(d.permitProcess);
    TEST_ASSERT_FALSE(d.permitDangerous);
    TEST_ASSERT_FALSE(d.permitHeater);
}

}  // namespace

void test_invalid_lease_without_history_is_deny() {
    const SafetyLease lease;
    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, SafetyConfig{});
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Deny),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_FALSE(d.latchRequest);
    assertNoPermits(d);
}

void test_full_valid_lease_allows_everything() {
    const SafetyDecision d = safetyPolicyEvaluate(
        fullLease(), SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Allow),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_TRUE(d.permitDangerous);
    TEST_ASSERT_TRUE(d.permitHeater);
    TEST_ASSERT_FALSE(d.latchRequest);
}

void test_lost_valid_after_ever_valid_latches() {
    SafetySupervisor sup;
    sup.reset(kNow);

    SafetyLease lease = fullLease();
    SafetyDecision d = sup.evaluate(lease, F_NONE, kNow, fullConfig());
    TEST_ASSERT_TRUE(d.permitDangerous);

    lease.valid = false;
    d = sup.evaluate(lease, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.latchRequest);
    TEST_ASSERT_EQUAL(static_cast<int>(StopReason::LinkLoss),
                      static_cast<int>(d.reason));
    assertNoPermits(d);
}

void test_stale_lease_after_ever_valid_latches() {
    SafetyHistory history;
    history.everValid = true;

    SafetyLease lease = fullLease();
    lease.rxStampMs = kNow - (kLeaseTimeoutMs + 1);

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, history, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_EQUAL(static_cast<int>(StopReason::LinkLoss),
                      static_cast<int>(d.reason));
}

void test_nano_latched_flag_latches() {
    SafetyLease lease = fullLease();
    lease.latched = true;
    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.latchRequest);
}

void test_any_alarm_bit_latches() {
    SafetyLease lease = fullLease();
    lease.alarmBits = 0x8000;   // неизвестный бит тоже запрещает
    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
}

void test_permit_lost_after_ever_permitted_latches() {
    SafetySupervisor sup;
    sup.reset(kNow);

    SafetyLease lease = fullLease();
    (void)sup.evaluate(lease, F_NONE, kNow, fullConfig());

    lease.permit = false;
    const SafetyDecision d = sup.evaluate(lease, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.latchRequest);
    TEST_ASSERT_EQUAL(static_cast<int>(StopReason::PermitLost),
                      static_cast<int>(d.reason));
}

void test_never_permitted_is_deny_not_latch() {
    SafetySupervisor sup;
    sup.reset(kNow);

    SafetyLease lease = fullLease();
    lease.permit = false;

    SafetyDecision d = sup.evaluate(lease, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Deny),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_FALSE(d.latchRequest);

    d = sup.evaluate(lease, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Deny),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_FALSE(d.latchRequest);
}

void test_gate_unknown_fault_and_latched() {
    SafetyLease lease = fullLease();

    lease.gateState = GateState::Unknown;
    SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_EQUAL(static_cast<int>(StopReason::GateFault),
                      static_cast<int>(d.reason));

    lease.gateState = GateState::Fault;
    d = safetyPolicyEvaluate(lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));

    lease.gateState = GateState::Latched;
    d = safetyPolicyEvaluate(lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
}

void test_gate_closed_allows_process_and_heater_but_not_danger() {
    SafetyLease lease = fullLease();
    lease.gateState = GateState::Closed;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Allow),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_FALSE(d.permitDangerous);
    TEST_ASSERT_TRUE(d.permitHeater);
}

void test_heater_requires_oil_min_and_temp() {
    SafetyLease lease = fullLease();
    lease.gateState = GateState::Closed;
    lease.sensorValid = kValidAirPressure | kValidOilTempSafety;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_FALSE(d.permitHeater);   // нет kValidFloatMin
    TEST_ASSERT_FALSE(d.permitDangerous);
}

void test_contactor_unknown_never_confirmed_allows_process_only() {
    SafetyLease lease = fullLease();
    lease.contactorFb = ContactorFb::Unknown;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_FALSE(d.permitDangerous);
    TEST_ASSERT_FALSE(d.permitHeater);
    TEST_ASSERT_FALSE(d.latchRequest);
}

void test_contactor_lost_after_confirmation_latches() {
    SafetyHistory history;
    history.everValid = true;
    history.everContactorConfirmed = true;

    SafetyLease lease = fullLease();
    lease.contactorFb = ContactorFb::Off;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, history, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
}

void test_power_absent_after_confirmation_latches() {
    SafetyHistory history;
    history.everValid = true;
    history.everPowerConfirmed = true;

    SafetyLease lease = fullLease();
    lease.kc868PowerFb = PowerFb::Absent;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, history, F_NONE, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
}

void test_power_absent_never_confirmed_allows_process_only() {
    SafetyLease lease = fullLease();
    lease.kc868PowerFb = PowerFb::Absent;

    const SafetyDecision d = safetyPolicyEvaluate(
        lease, SafetyHistory{}, F_NONE, kNow, fullConfig());
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_FALSE(d.permitDangerous);
    TEST_ASSERT_FALSE(d.permitHeater);
}

void test_local_high_fault_latches_independently() {
    const SafetyDecision d = safetyPolicyEvaluate(
        fullLease(), SafetyHistory{}, F_FLAME_LOSS, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Latch),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.latchRequest);
    TEST_ASSERT_EQUAL(static_cast<int>(StopReason::FlameLoss),
                      static_cast<int>(d.reason));
    assertNoPermits(d);
}

void test_low_or_medium_fault_does_not_latch() {
    const SafetyDecision d = safetyPolicyEvaluate(
        fullLease(), SafetyHistory{}, F_IGNITION_TIMEOUT, kNow, fullConfig());
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Allow),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.permitDangerous);
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_invalid_lease_without_history_is_deny);
    RUN_TEST(test_full_valid_lease_allows_everything);
    RUN_TEST(test_lost_valid_after_ever_valid_latches);
    RUN_TEST(test_stale_lease_after_ever_valid_latches);
    RUN_TEST(test_nano_latched_flag_latches);
    RUN_TEST(test_any_alarm_bit_latches);
    RUN_TEST(test_permit_lost_after_ever_permitted_latches);
    RUN_TEST(test_never_permitted_is_deny_not_latch);
    RUN_TEST(test_gate_unknown_fault_and_latched);
    RUN_TEST(test_gate_closed_allows_process_and_heater_but_not_danger);
    RUN_TEST(test_heater_requires_oil_min_and_temp);
    RUN_TEST(test_contactor_unknown_never_confirmed_allows_process_only);
    RUN_TEST(test_contactor_lost_after_confirmation_latches);
    RUN_TEST(test_power_absent_after_confirmation_latches);
    RUN_TEST(test_power_absent_never_confirmed_allows_process_only);
    RUN_TEST(test_local_high_fault_latches_independently);
    RUN_TEST(test_low_or_medium_fault_does_not_latch);

    return UNITY_END();
}

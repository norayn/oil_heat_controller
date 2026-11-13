#include <unity.h>

#if OILCTRL_IS_NANO_STUB

#include "bench/nano_stub_link.h"
#include "domain/safety_policy.h"
#include "domain/safety_supervisor.h"

void test_nano_stub_lease_is_all_ok() {
    NanoStubLink stub;
    stub.poll(1000);

    const SafetyLease& lease = stub.lease();
    TEST_ASSERT_TRUE(lease.valid);
    TEST_ASSERT_TRUE(lease.permit);
    TEST_ASSERT_FALSE(lease.latched);
    TEST_ASSERT_EQUAL(static_cast<int>(GateState::OpenSafe),
                      static_cast<int>(lease.gateState));
    TEST_ASSERT_EQUAL_UINT16(0, lease.alarmBits);
    TEST_ASSERT_EQUAL_UINT16(kRequiredSensors, lease.sensorValid);
    TEST_ASSERT_EQUAL(static_cast<int>(ContactorFb::OnConfirmed),
                      static_cast<int>(lease.contactorFb));
    TEST_ASSERT_EQUAL(static_cast<int>(PowerFb::Present),
                      static_cast<int>(lease.kc868PowerFb));
    TEST_ASSERT_EQUAL_UINT32(1000, lease.rxStampMs);
    TEST_ASSERT_TRUE(leaseFresh(lease, 1100));
}

void test_stub_drives_supervisor_to_full_allow() {
    NanoStubLink stub;
    SafetySupervisor sup;
    const uint32_t now = 5000;

    stub.poll(now);
    sup.reset(now);

    SafetyConfig cfg;
    cfg.requireContactorFb = true;
    cfg.requireKc868PowerFb = true;

    const SafetyDecision d = sup.evaluate(stub.lease(), F_NONE, now, cfg);
    TEST_ASSERT_EQUAL(static_cast<int>(SafetyVerdict::Allow),
                      static_cast<int>(d.verdict));
    TEST_ASSERT_TRUE(d.permitProcess);
    TEST_ASSERT_TRUE(d.permitDangerous);
    TEST_ASSERT_TRUE(d.permitHeater);
    TEST_ASSERT_FALSE(d.latchRequest);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_nano_stub_lease_is_all_ok);
    RUN_TEST(test_stub_drives_supervisor_to_full_allow);
    return UNITY_END();
}

#else

void test_stub_tests_do_not_build_without_nano_stub() {
    TEST_ASSERT_FALSE(OILCTRL_IS_NANO_STUB);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_stub_tests_do_not_build_without_nano_stub);
    return UNITY_END();
}

#endif

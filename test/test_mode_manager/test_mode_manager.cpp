#include <unity.h>

#include "domain/mode_manager.h"

namespace {
constexpr uint32_t kNow = 1000;
}

void test_boot_safe_then_off() {
    ModeManager mode;
    mode.begin(kNow);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::BootSafe),
                      static_cast<int>(mode.mode()));

    mode.finishSelfCheck(kNow, true);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

void test_self_check_failure_goes_fault() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, false);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Fault),
                      static_cast<int>(mode.mode()));
}

void test_manual_request_false_never_enters_manual() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);

    TEST_ASSERT_FALSE(mode.requestManual(kNow, false));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

void test_manual_round_trip() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);

    TEST_ASSERT_TRUE(mode.requestManual(kNow, true));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Manual),
                      static_cast<int>(mode.mode()));

    // Auto из Manual не входит.
    TEST_ASSERT_FALSE(mode.requestAuto(kNow, true, false));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Manual),
                      static_cast<int>(mode.mode()));

    mode.leaveManual(kNow);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

void test_auto_requires_permit_and_no_latch() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);

    TEST_ASSERT_FALSE(mode.requestAuto(kNow, false, false));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));

    TEST_ASSERT_FALSE(mode.requestAuto(kNow, true, true));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));

    TEST_ASSERT_TRUE(mode.requestAuto(kNow, true, false));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Auto),
                      static_cast<int>(mode.mode()));
}

void test_latch_is_record_and_reset_returns_off() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);
    mode.requestAuto(kNow, true, false);

    mode.recordLatch(kNow);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Latched),
                      static_cast<int>(mode.mode()));

    TEST_ASSERT_TRUE(mode.resetLatch(kNow));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

void test_force_off_from_manual() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);
    mode.requestManual(kNow, true);
    mode.forceOff(kNow);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

void test_latch_is_ignored_in_boot_safe() {
    ModeManager mode;
    mode.begin(kNow);
    mode.recordLatch(kNow);
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::BootSafe),
                      static_cast<int>(mode.mode()));
}

void test_reset_without_latch_does_nothing() {
    ModeManager mode;
    mode.begin(kNow);
    mode.finishSelfCheck(kNow, true);
    TEST_ASSERT_FALSE(mode.resetLatch(kNow));
    TEST_ASSERT_EQUAL(static_cast<int>(RunMode::Off),
                      static_cast<int>(mode.mode()));
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_boot_safe_then_off);
    RUN_TEST(test_self_check_failure_goes_fault);
    RUN_TEST(test_manual_request_false_never_enters_manual);
    RUN_TEST(test_manual_round_trip);
    RUN_TEST(test_auto_requires_permit_and_no_latch);
    RUN_TEST(test_latch_is_record_and_reset_returns_off);
    RUN_TEST(test_force_off_from_manual);
    RUN_TEST(test_latch_is_ignored_in_boot_safe);
    RUN_TEST(test_reset_without_latch_does_nothing);

    return UNITY_END();
}

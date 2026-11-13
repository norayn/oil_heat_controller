#include <unity.h>

#include "config/limits.h"
#include "hal/dummy_out.h"
#include "manual/manual_console.h"
#include "manual/manual_control.h"
#include "services/actuator_service.h"

#if OILCTRL_IS_MANUAL

namespace {

constexpr uint8_t kCount = static_cast<uint8_t>(ActuatorId::Count);

struct Rig {
    DummyOut outs[kCount];
    ActuatorService svc;
    ManualControlService manual;

    Rig() {
        for (uint8_t i = 0; i < kCount; ++i) {
            svc.bind(static_cast<ActuatorId>(i), &outs[i]);
        }
        svc.beginSafe();
        manual.begin(&svc, 0);
    }

    bool on(ActuatorId id) const { return outs[static_cast<uint8_t>(id)].on(); }
};

}  // namespace

void test_enter_requires_all_outputs_off() {
    Rig rig;
    TEST_ASSERT_TRUE(rig.manual.enter(0));

    Rig dirty;
    dirty.outs[static_cast<uint8_t>(ActuatorId::FanBlower)].write(true);
    TEST_ASSERT_FALSE(dirty.manual.enter(0));
}

void test_enter_rejected_when_outputs_unknown() {
    Rig rig;
    rig.svc.setOutputsUnknown(true);
    TEST_ASSERT_FALSE(rig.manual.enter(0));
}

void test_set_one_writes_after_tick() {
    Rig rig;
    TEST_ASSERT_TRUE(rig.manual.enter(0));
    TEST_ASSERT_TRUE(rig.manual.setManual(ActuatorId::FanBlower, true, 0));

    // setManual не пишет в железо: запись только в tick().
    TEST_ASSERT_FALSE(rig.on(ActuatorId::FanBlower));
    rig.manual.tick(1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::FanBlower));
}

void test_two_commands_share_one_vector() {
    Rig rig;
    rig.manual.enter(0);

    rig.manual.setManual(ActuatorId::FanBlower, true, 0);
    rig.manual.tick(1);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 2);
    rig.manual.tick(2);

    TEST_ASSERT_TRUE(rig.on(ActuatorId::FanBlower));
    TEST_ASSERT_TRUE(rig.on(ActuatorId::OilValveMain));
    TEST_ASSERT_EQUAL_UINT16(
        actuatorBit(ActuatorId::FanBlower) | actuatorBit(ActuatorId::OilValveMain),
        rig.manual.bits());
}

void test_heater_allowed_without_can_heat_oil() {
    Rig rig;
    rig.manual.enter(0);
    TEST_ASSERT_TRUE(rig.manual.setManual(ActuatorId::HeaterOil, true, 0));
    rig.manual.tick(1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::HeaterOil));
}

void test_deadman_turns_dangerous_group_off() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 0);

    rig.manual.tick(1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::OilValveMain));

    rig.manual.tick(kManualDeadmanMs - 1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::OilValveMain));

    rig.manual.tick(kManualDeadmanMs);
    TEST_ASSERT_FALSE(rig.on(ActuatorId::OilValveMain));
}

void test_keep_extends_deadman() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 0);

    rig.manual.tick(9000);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::OilValveMain));

    rig.manual.keep(9000);
    rig.manual.tick(9000 + kManualDeadmanMs - 1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::OilValveMain));

    rig.manual.tick(9000 + kManualDeadmanMs);
    TEST_ASSERT_FALSE(rig.on(ActuatorId::OilValveMain));
}

void test_repeated_on_does_not_extend_window() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 0);
    rig.manual.tick(1);

    TEST_ASSERT_TRUE(rig.manual.setManual(ActuatorId::OilValveMain, true, 9000));
    rig.manual.tick(kManualDeadmanMs);
    TEST_ASSERT_FALSE(rig.on(ActuatorId::OilValveMain));
}

void test_max_on_turns_process_group_off() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::FanBlower, true, 0);
    rig.manual.tick(1);
    TEST_ASSERT_TRUE(rig.on(ActuatorId::FanBlower));

    rig.manual.tick(kManualMaxOnFanMs);
    TEST_ASSERT_FALSE(rig.on(ActuatorId::FanBlower));
}

void test_min_off_blocks_re_enable() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 0);
    rig.manual.tick(1);

    rig.manual.setManual(ActuatorId::OilValveMain, false, 100);
    TEST_ASSERT_FALSE(rig.manual.setManual(ActuatorId::OilValveMain, true, 600));
    TEST_ASSERT_TRUE(rig.manual.setManual(ActuatorId::OilValveMain, true, 1100));
}

void test_all_off_exits_manual() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::FanBlower, true, 0);
    rig.manual.tick(1);

    rig.manual.allOffManual(2);
    TEST_ASSERT_FALSE(rig.manual.enabled());
    TEST_ASSERT_EQUAL_UINT16(kAllOff, rig.manual.bits());
    TEST_ASSERT_FALSE(rig.on(ActuatorId::FanBlower));
}

void test_i2c_fault_forces_all_off_and_exit() {
    Rig rig;
    rig.manual.enter(0);
    rig.manual.setManual(ActuatorId::OilValveMain, true, 0);
    rig.manual.tick(1);

    rig.svc.setOutputsUnknown(true);
    rig.manual.tick(2);

    TEST_ASSERT_FALSE(rig.manual.enabled());
    TEST_ASSERT_EQUAL_UINT16(kAllOff, rig.manual.bits());
    TEST_ASSERT_FALSE(rig.on(ActuatorId::OilValveMain));
}

void test_unknown_actuator_is_rejected() {
    Rig rig;
    rig.manual.enter(0);
    TEST_ASSERT_FALSE(rig.manual.setManual(
        static_cast<ActuatorId>(200), true, 0));
}

void test_console_parses_commands() {
    ManualCommand cmd;

    TEST_ASSERT_TRUE(parseManualCommand("manual on", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::ManualOn),
                      static_cast<int>(cmd.verb));

    TEST_ASSERT_TRUE(parseManualCommand("  manual   off ", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::ManualOff),
                      static_cast<int>(cmd.verb));

    TEST_ASSERT_TRUE(parseManualCommand("set fan 1", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::SetOne),
                      static_cast<int>(cmd.verb));
    TEST_ASSERT_EQUAL(static_cast<int>(ActuatorId::FanBlower),
                      static_cast<int>(cmd.actuator));
    TEST_ASSERT_TRUE(cmd.value);

    TEST_ASSERT_TRUE(parseManualCommand("set pump_transfer 0", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ActuatorId::PumpOilTransfer),
                      static_cast<int>(cmd.actuator));
    TEST_ASSERT_FALSE(cmd.value);

    TEST_ASSERT_TRUE(parseManualCommand("set all 0", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::AllOff),
                      static_cast<int>(cmd.verb));

    TEST_ASSERT_TRUE(parseManualCommand("keep", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::Keep),
                      static_cast<int>(cmd.verb));

    TEST_ASSERT_TRUE(parseManualCommand("status", cmd));
    TEST_ASSERT_EQUAL(static_cast<int>(ManualVerb::Status),
                      static_cast<int>(cmd.verb));

    TEST_ASSERT_FALSE(parseManualCommand("set all 1", cmd));
    TEST_ASSERT_FALSE(parseManualCommand("set unknown 1", cmd));
    TEST_ASSERT_FALSE(parseManualCommand("manual maybe", cmd));
    TEST_ASSERT_FALSE(parseManualCommand("", cmd));
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_enter_requires_all_outputs_off);
    RUN_TEST(test_enter_rejected_when_outputs_unknown);
    RUN_TEST(test_set_one_writes_after_tick);
    RUN_TEST(test_two_commands_share_one_vector);
    RUN_TEST(test_heater_allowed_without_can_heat_oil);
    RUN_TEST(test_deadman_turns_dangerous_group_off);
    RUN_TEST(test_keep_extends_deadman);
    RUN_TEST(test_repeated_on_does_not_extend_window);
    RUN_TEST(test_max_on_turns_process_group_off);
    RUN_TEST(test_min_off_blocks_re_enable);
    RUN_TEST(test_all_off_exits_manual);
    RUN_TEST(test_i2c_fault_forces_all_off_and_exit);
    RUN_TEST(test_unknown_actuator_is_rejected);
    RUN_TEST(test_console_parses_commands);

    return UNITY_END();
}

#else

void test_manual_tests_do_not_build_in_field_profile() {
    TEST_ASSERT_FALSE(OILCTRL_IS_MANUAL);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_manual_tests_do_not_build_in_field_profile);
    return UNITY_END();
}

#endif

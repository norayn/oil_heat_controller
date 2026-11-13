#include <unity.h>

#include "hal/dummy_out.h"
#include "services/actuator_service.h"

namespace {

ActuatorDemand demandFor(BurnerState state, ActuatorBits bits) {
    ActuatorDemand d;
    d.state = state;
    d.bits = bits;
    d.permitDangerous = true;
    d.permitHeater = true;
    d.allowHeatOil = true;
    return d;
}

struct StuckOnOut final : public IDigitalOut {
    void write(bool) override {}
    Readback readback() const override { return Readback::On; }
};

struct Rig {
    DummyOut outs[static_cast<uint8_t>(ActuatorId::Count)];
    ActuatorService svc;

    Rig() {
        for (uint8_t i = 0; i < static_cast<uint8_t>(ActuatorId::Count); ++i) {
            svc.bind(static_cast<ActuatorId>(i), &outs[i]);
        }
        svc.beginSafe();
    }
};

}  // namespace

void test_default_demand_is_fail_closed() {
    const ActuatorDemand d;
    TEST_ASSERT_EQUAL_UINT16(kAllOff, d.bits);
    TEST_ASSERT_FALSE(d.permitDangerous);
    TEST_ASSERT_FALSE(d.permitHeater);
    TEST_ASSERT_FALSE(d.allowHeatOil);
}

void test_init_forces_all_off() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Init, kSafetyOffGroup | actuatorBit(ActuatorId::FanBlower)),
        violated);
    TEST_ASSERT_EQUAL_UINT16(kAllOff, out);
    TEST_ASSERT_TRUE(violated);
}

void test_fault_strips_safety_off_group_but_keeps_fan() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Fault,
                  kSafetyOffGroup | actuatorBit(ActuatorId::FanBlower)),
        violated);
    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), out);
    TEST_ASSERT_TRUE(violated);
}

void test_pre_purge_has_no_air_valve() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::PrePurge,
                  actuatorBit(ActuatorId::FanBlower) | kAirPath),
        violated);
    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), out);
    TEST_ASSERT_TRUE(violated);
}

void test_idle_strips_fuel_spark_air() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Idle,
                  actuatorBit(ActuatorId::FanBlower) | kFuelGroup | kSparkGroup | kAirPath),
        violated);
    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), out);
    TEST_ASSERT_TRUE(violated);
}

void test_idle_heater_requires_allow_heat_oil() {
    ActuatorDemand d = demandFor(
        BurnerState::Idle,
        actuatorBit(ActuatorId::FanBlower) | kHeaterGroup);
    d.allowHeatOil = false;

    bool violated = false;
    ActuatorBits out = ActuatorService::applyInvariant(d, violated);
    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), out);
    TEST_ASSERT_FALSE(violated);   // штатная fail-closed маска

    d.allowHeatOil = true;
    out = ActuatorService::applyInvariant(d, violated);
    TEST_ASSERT_EQUAL_UINT16(
        actuatorBit(ActuatorId::FanBlower) | kHeaterGroup, out);
}

void test_ignition_strips_heater_and_transfer() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Ignition,
                  actuatorBit(ActuatorId::FanBlower) | kSparkGroup | kCombustion |
                  kHeaterGroup | actuatorBit(ActuatorId::PumpOilTransfer)),
        violated);
    TEST_ASSERT_EQUAL_UINT16(
        actuatorBit(ActuatorId::FanBlower) | kSparkGroup | kCombustion, out);
    TEST_ASSERT_TRUE(violated);
}

void test_run_valid_demand_passes() {
    bool violated = false;
    const ActuatorDemand d = demandFor(
        BurnerState::Run,
        actuatorBit(ActuatorId::FanBlower) | kAirPath | actuatorBit(ActuatorId::OilValveMain));
    const ActuatorBits out = ActuatorService::applyInvariant(d, violated);
    TEST_ASSERT_EQUAL_UINT16(d.bits, out);
    TEST_ASSERT_FALSE(violated);
}

void test_run_without_fan_drops_fuel() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Run,
                  kAirPath | actuatorBit(ActuatorId::OilValveMain)),
        violated);
    TEST_ASSERT_EQUAL_UINT16(kAirPath, out);
    TEST_ASSERT_TRUE(violated);
}

void test_run_strips_spark_and_transfer() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Run,
                  actuatorBit(ActuatorId::FanBlower) | kAirPath |
                  actuatorBit(ActuatorId::OilValveMain) | kSparkGroup |
                  actuatorBit(ActuatorId::PumpOilTransfer)),
        violated);
    TEST_ASSERT_EQUAL_UINT16(
        actuatorBit(ActuatorId::FanBlower) | kAirPath | actuatorBit(ActuatorId::OilValveMain),
        out);
    TEST_ASSERT_TRUE(violated);
}

void test_run_heater_follows_permit_and_allow() {
    ActuatorDemand d = demandFor(
        BurnerState::Run,
        actuatorBit(ActuatorId::FanBlower) | kAirPath |
        actuatorBit(ActuatorId::OilValveMain) | kHeaterGroup);

    bool violated = false;
    ActuatorBits out = ActuatorService::applyInvariant(d, violated);
    TEST_ASSERT_TRUE((out & kHeaterGroup) != 0);

    d.permitHeater = false;
    out = ActuatorService::applyInvariant(d, violated);
    TEST_ASSERT_TRUE((out & kHeaterGroup) == 0);
}

void test_no_permit_dangerous_strips_danger_group() {
    ActuatorDemand d = demandFor(
        BurnerState::Run,
        actuatorBit(ActuatorId::FanBlower) | kAirPath | actuatorBit(ActuatorId::OilValveMain));
    d.permitDangerous = false;

    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(d, violated);
    // kDangerGroup включает AirValveMain; остаётся только вентилятор.
    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), out);
}

void test_unknown_bit_rejects_whole_command() {
    bool violated = false;
    const ActuatorBits out = ActuatorService::applyInvariant(
        demandFor(BurnerState::Run,
                  actuatorBit(ActuatorId::FanBlower) | 0x8000),
        violated);
    TEST_ASSERT_EQUAL_UINT16(kAllOff, out);
    TEST_ASSERT_TRUE(violated);
}

void test_tick_auto_writes_to_ports() {
    Rig rig;
    const ActuatorDemand d = demandFor(
        BurnerState::Run,
        actuatorBit(ActuatorId::FanBlower) | kAirPath | actuatorBit(ActuatorId::OilValveMain));

    rig.svc.tickAuto(10, d);
    TEST_ASSERT_EQUAL_UINT16(d.bits, rig.svc.applied());
    TEST_ASSERT_TRUE(rig.outs[static_cast<uint8_t>(ActuatorId::FanBlower)].on());
    TEST_ASSERT_TRUE(rig.outs[static_cast<uint8_t>(ActuatorId::AirValveMain)].on());
    TEST_ASSERT_TRUE(rig.outs[static_cast<uint8_t>(ActuatorId::OilValveMain)].on());
    TEST_ASSERT_FALSE(rig.outs[static_cast<uint8_t>(ActuatorId::HeaterOil)].on());
    TEST_ASSERT_FALSE(rig.svc.lastDemandViolated());
}

void test_begin_safe_forces_all_off() {
    Rig rig;
    for (uint8_t i = 0; i < static_cast<uint8_t>(ActuatorId::Count); ++i) {
        rig.outs[i].write(true);
    }
    rig.svc.beginSafe();

    TEST_ASSERT_EQUAL_UINT16(kAllOff, rig.svc.applied());
    for (uint8_t i = 0; i < static_cast<uint8_t>(ActuatorId::Count); ++i) {
        TEST_ASSERT_FALSE(rig.outs[i].on());
    }
    TEST_ASSERT_FALSE(rig.svc.outputsUnknown());
}

void test_outputs_unknown_strips_safety_off_group() {
    Rig rig;
    rig.svc.setOutputsUnknown(true);

    rig.svc.tickAuto(10, demandFor(
        BurnerState::Run,
        actuatorBit(ActuatorId::FanBlower) | kAirPath |
        actuatorBit(ActuatorId::OilValveMain) | kHeaterGroup));

    TEST_ASSERT_EQUAL_UINT16(actuatorBit(ActuatorId::FanBlower), rig.svc.applied());
}

void test_readback_mismatch_detected() {
    StuckOnOut stuck;
    ActuatorService svc;
    svc.bind(ActuatorId::HeaterOil, &stuck);
    svc.beginSafe();

    TEST_ASSERT_TRUE(svc.outputsUnknown());
    svc.setOutputsUnknown(false);
    TEST_ASSERT_TRUE(svc.readbackMismatch());
    TEST_ASSERT_EQUAL(static_cast<int>(Readback::On),
                      static_cast<int>(svc.readback(ActuatorId::HeaterOil)));
}

void test_unbound_output_is_unknown() {
    ActuatorService svc;
    svc.beginSafe();
    TEST_ASSERT_TRUE(svc.outputsUnknown());
    TEST_ASSERT_EQUAL(static_cast<int>(Readback::NotAvailable),
                      static_cast<int>(svc.readback(ActuatorId::FanBlower)));
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_default_demand_is_fail_closed);
    RUN_TEST(test_init_forces_all_off);
    RUN_TEST(test_fault_strips_safety_off_group_but_keeps_fan);
    RUN_TEST(test_pre_purge_has_no_air_valve);
    RUN_TEST(test_idle_strips_fuel_spark_air);
    RUN_TEST(test_idle_heater_requires_allow_heat_oil);
    RUN_TEST(test_ignition_strips_heater_and_transfer);
    RUN_TEST(test_run_valid_demand_passes);
    RUN_TEST(test_run_without_fan_drops_fuel);
    RUN_TEST(test_run_strips_spark_and_transfer);
    RUN_TEST(test_run_heater_follows_permit_and_allow);
    RUN_TEST(test_no_permit_dangerous_strips_danger_group);
    RUN_TEST(test_unknown_bit_rejects_whole_command);
    RUN_TEST(test_tick_auto_writes_to_ports);
    RUN_TEST(test_begin_safe_forces_all_off);
    RUN_TEST(test_outputs_unknown_strips_safety_off_group);
    RUN_TEST(test_readback_mismatch_detected);
    RUN_TEST(test_unbound_output_is_unknown);

    return UNITY_END();
}

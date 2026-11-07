#pragma once
#include <Arduino.h>
#include <stdint.h>

// SAFETY DRAFT — DO NOT USE IN FIRMWARE UNTIL `safety-reviewer` APPROVE.
// Dedicated point-to-point UART safety channel: Main Unit (KC868/ESP32-S3) <-> Safety Unit (Nano).
// Replaces non-safety TelemetryPacket placeholder.
// Rules: fail-safe on CRC/seq/protoVer/timeout/invalid fields; no dangerous auto-resume after latch;
// heartbeat 100-200 ms, timeout <= 3 periods (<=600 ms); see docs/protocol/uart.md and
// docs/architecture/blockers.md (BLK-006). Any implementation in src_esp32/src_avr8 requires
// safety-reviewer.

namespace SafetyProtocol {

static constexpr uint8_t PROTO_VER = 1;
static constexpr uint8_t START_MAIN_TO_SAFETY = 0xA5;
static constexpr uint8_t START_SAFETY_TO_MAIN = 0x5A;

// Main -> Safety FSM states (minimal; extend only with safety-reviewer).
enum class MainFsmState : uint8_t {
    UNKNOWN = 0,
    INIT = 1,
    IDLE = 2,
    PREPURGE = 3,
    IGNITION = 4,
    RUN = 5,
    POSTPURGE = 6,
    SHUTDOWN = 7,
    FAULT = 8,
    LATCHED = 9,
};

// Nano gate/safety states.
enum class SafetyGateState : uint8_t {
    UNKNOWN = 0,
    CLOSED = 1,      // fail-safe
    OPEN_SAFE = 2,   // Nano permits only when conditions valid
    LATCHED = 3,
    FAULT = 4,
};

// Main -> Safety i2cStatus
enum I2cStatusBits : uint8_t {
    I2C_OK = 0x00,
    I2C_FAULT = 0x01,
    I2C_RECOVERED = 0x02,
};

// Main -> Safety actuator/status bits (requested/observed by Main Unit; Nano has final authority).
enum MainActBits : uint16_t {
    ACT_OIL_VALVE_MAIN_REQ    = 1u << 0,
    ACT_AIR_VALVE_MAIN_REQ    = 1u << 1,
    ACT_HEATER_OIL_REQ        = 1u << 2,
    ACT_PUMP_OIL_TRANSFER_REQ = 1u << 3,
    ACT_IGNITION_UNIT_REQ     = 1u << 4,
    ACT_FAN_BLOWER_REQ        = 1u << 5,
    ACT_PUMP_BOILER_REQ       = 1u << 6,
    // bits 7..15 reserved
};

// Main -> Safety flame/ignition status (supervision aid; Nano has no direct flame input unless design changes).
enum FlameStatus : uint8_t {
    FLAME_INVALID = 0,
    FLAME_OFF = 1,
    FLAME_ON = 2,
    FLAME_DEGRADED = 3, // sensor/EMI/diagnostic uncertainty => treat conservatively
};

// Nano -> Main alarm bits (extend with versioning/safety-reviewer).
enum AlarmBits : uint16_t {
    ALARM_NONE                = 0,
    ALARM_HEARTBEAT_TIMEOUT   = 1u << 0,
    ALARM_CRC_SEQ_VER         = 1u << 1,
    ALARM_AIR_PRESSURE        = 1u << 2,
    ALARM_OIL_TEMP_SAFETY     = 1u << 3,
    ALARM_FLOAT_MIN           = 1u << 4,
    ALARM_FLOAT_MAX           = 1u << 5,
    ALARM_MAIN_I2C_FAULT      = 1u << 6,
    ALARM_SEMANTIC_STALL      = 1u << 7,
    ALARM_CONTACTOR_FEEDBACK  = 1u << 8,
    ALARM_KC868_POWER_FAULT   = 1u << 9,
    ALARM_MANUAL_LATCH        = 1u << 10,
    // bits 11..15 reserved
};

// Nano -> Main sensor validity bits.
enum SensorValidBits : uint16_t {
    VALID_AIR_PRESSURE    = 1u << 0,
    VALID_OIL_TEMP_SAFETY = 1u << 1,
    VALID_FLOAT_MIN       = 1u << 2,
    VALID_FLOAT_MAX       = 1u << 3,
    VALID_CONTACTOR_FB    = 1u << 4,
    VALID_KC868_POWER_FB  = 1u << 5,
    // bits 6..15 reserved
};

// Nano -> Main KC868 power feedback (SR-B6 / BLK-001 mitigation context).
enum Kc868PowerFb : uint8_t {
    KC868_PWR_UNKNOWN = 0,
    KC868_PWR_PRESENT = 1,
    KC868_PWR_ABSENT  = 2,
    KC868_PWR_FAULT   = 3, // e.g. commanded off but feedback disagrees / weld / stuck
};

// Nano -> Main contactor feedback (BLK-002). FAIL-SAFE POLARITY required in schematic:
// unknown/loss/broken feedback must NOT be reported as OFF_CONFIRMED.
enum ContactorFb : uint8_t {
    CONTACTOR_FB_UNKNOWN       = 0,
    CONTACTOR_FB_OFF_CONFIRMED = 1, // only with valid active confirmation scheme
    CONTACTOR_FB_ON_CONFIRMED  = 2,
    CONTACTOR_FB_FAULT         = 3, // mismatch/stuck/weld suspected
};

struct __attribute__((packed)) HeartbeatMainToSafety {
    uint8_t  start = START_MAIN_TO_SAFETY;
    uint8_t  protoVer = PROTO_VER;
    uint8_t  seq = 0;                 // monotonic mod 256
    uint8_t  fsmState = static_cast<uint8_t>(MainFsmState::UNKNOWN);
    uint16_t stateAgeMs = 0;          // capped/saturated
    uint16_t fsmCycle = 0;            // increments at completed FSM step (per uart.md)
    uint16_t i2cPollCycle = 0;        // increments only on successful I2C transaction+read-back
    uint8_t  i2cStatus = I2C_OK;
    uint8_t  flameStatus = FLAME_INVALID;
    uint16_t actBits = 0;             // requested actuator bits
    uint8_t  errSeverity = 0;         // 0=none; project error severity mapping TBD
    uint8_t  reserved = 0;
    uint8_t  crc8 = 0;                // CRC over preceding bytes (algorithm TBD, must be fixed before impl)
};

struct __attribute__((packed)) HeartbeatSafetyToMain {
    uint8_t  start = START_SAFETY_TO_MAIN;
    uint8_t  protoVer = PROTO_VER;
    uint8_t  seq = 0;
    uint8_t  gateState = static_cast<uint8_t>(SafetyGateState::UNKNOWN);
    uint8_t  safetyPermit = 0;        // 0 = no permit / safe state required
    uint8_t  latch = 0;               // 0 = no latch
    uint16_t alarmBits = 0;
    uint16_t sensorValid = 0;
    uint8_t  kc868PowerFb = KC868_PWR_UNKNOWN;
    uint8_t  contactorFb = CONTACTOR_FB_UNKNOWN; // fail-safe polarity: unknown!=off-confirmed
    uint8_t  errSeverity = 0;
    uint8_t  reserved = 0;
    uint8_t  crc8 = 0;
};

static_assert(sizeof(HeartbeatMainToSafety) == 18, "HeartbeatMainToSafety size changed; update protocol docs/version");
static_assert(sizeof(HeartbeatSafetyToMain) == 16, "HeartbeatSafetyToMain size changed; update protocol docs/version");

} // namespace SafetyProtocol

#pragma once
// Режим работы Main Unit (§8.4).
// ModeManager не знает про IManualRequest: app опрашивает порт и передаёт bool.
// RunMode::Manual присутствует в enum ВСЕГДА (ADR-11); гейтится только вызов.

#include <cstdint>

enum class RunMode : uint8_t {
    BootSafe = 0,
    Off,
    Manual,
    Auto,
    Fault,
    Latched
};

class ModeManager {
public:
    void begin(uint32_t nowMs);
    void finishSelfCheck(uint32_t nowMs, bool ok);

    RunMode mode() const { return mode_; }

    // Off <-> Manual. В FIELD вызывающий передаёт false (NullManualRequest),
    // поэтому перехода в Manual нет.
    bool requestManual(uint32_t nowMs, bool requested);
    void leaveManual(uint32_t nowMs);

    // Off -> Auto только при permitProcess и отсутствии latch.
    // heatRequest не требуется: предпусковой подогрев в Idle достижим.
    bool requestAuto(uint32_t nowMs, bool permitProcess, bool latched);

    // Отражение состояния FSM/аварии, а не второй источник demand.
    void forceFault(uint32_t nowMs);
    void recordLatch(uint32_t nowMs);
    bool resetLatch(uint32_t nowMs);
    void forceOff(uint32_t nowMs);

private:
    RunMode mode_{RunMode::BootSafe};
};

constexpr const char* runModeName(RunMode mode) {
    switch (mode) {
        case RunMode::BootSafe: return "BootSafe";
        case RunMode::Off:      return "Off";
        case RunMode::Manual:   return "Manual";
        case RunMode::Auto:     return "Auto";
        case RunMode::Fault:    return "Fault";
        case RunMode::Latched:  return "Latched";
    }
    return "UNKNOWN";
}

#pragma once
// Планировщик и периодические таймеры (§6). Host-clean.
// millis() wrap-safe; пропущенные тики не догоняются пачкой.

#include <cstdint>

class PeriodicTimer {
public:
    void start(uint32_t nowMs, uint32_t periodMs);
    void stop();

    // true — ровно один раз за период; после срабатывания следующий срок
    // назначается от nowMs, а не от прошлого срока (без догоняющей пачки).
    bool due(uint32_t nowMs);

    uint32_t periodMs() const { return period_; }
    bool     armed() const { return armed_; }
    uint32_t missed() const { return missed_; }
    void     clearMissed() { missed_ = 0; }

private:
    uint32_t period_{1};
    uint32_t next_{0};
    uint32_t missed_{0};
    bool     armed_{false};
};

class Scheduler {
public:
    static constexpr uint8_t kMaxTasks = 12;

    void begin(uint32_t nowMs);
    bool addTask(uint8_t id, uint32_t periodMs, uint32_t nowMs);
    bool due(uint8_t id, uint32_t nowMs);
    uint32_t missedTicks(uint8_t id) const;

private:
    struct Entry {
        PeriodicTimer timer;
        bool          used{false};
    };

    Entry tasks_[kMaxTasks]{};
};

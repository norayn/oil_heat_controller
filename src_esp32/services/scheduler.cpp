#include "services/scheduler.h"

void PeriodicTimer::start(uint32_t nowMs, uint32_t periodMs) {
    period_ = periodMs == 0 ? 1 : periodMs;
    next_ = nowMs + period_;
    missed_ = 0;
    armed_ = true;
}

void PeriodicTimer::stop() {
    armed_ = false;
}

bool PeriodicTimer::due(uint32_t nowMs) {
    if (!armed_) {
        return false;
    }

    // Знаковая разность корректна и через переполнение uint32.
    const int32_t delta = static_cast<int32_t>(nowMs - next_);
    if (delta < 0) {
        return false;
    }

    const uint32_t lateMs = static_cast<uint32_t>(delta);
    missed_ += lateMs / period_;
    next_ = nowMs + period_;
    return true;
}

void Scheduler::begin(uint32_t nowMs) {
    (void)nowMs;
    for (uint8_t i = 0; i < kMaxTasks; ++i) {
        tasks_[i] = Entry{};
    }
}

bool Scheduler::addTask(uint8_t id, uint32_t periodMs, uint32_t nowMs) {
    if (id >= kMaxTasks) {
        return false;
    }
    tasks_[id].used = true;
    tasks_[id].timer.start(nowMs, periodMs);
    return true;
}

bool Scheduler::due(uint8_t id, uint32_t nowMs) {
    if (id >= kMaxTasks || !tasks_[id].used) {
        return false;
    }
    return tasks_[id].timer.due(nowMs);
}

uint32_t Scheduler::missedTicks(uint8_t id) const {
    if (id >= kMaxTasks || !tasks_[id].used) {
        return 0;
    }
    return tasks_[id].timer.missed();
}

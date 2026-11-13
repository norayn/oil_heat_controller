#pragma once
// Простейший цифровой выход для host-тестов и bench-заглушек.

#include "hal/ports.h"

class DummyOut final : public IDigitalOut {
public:
    void write(bool on) override {
        on_ = on;
        writes_ += 1;
    }

    Readback readback() const override {
        return on_ ? Readback::On : Readback::Off;
    }

    bool on() const { return on_; }
    uint32_t writes() const { return writes_; }

private:
    bool     on_{false};
    uint32_t writes_{0};
};

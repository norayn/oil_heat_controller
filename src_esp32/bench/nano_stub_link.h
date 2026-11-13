#pragma once
// Заглушка Nano для bench-auto (AR-05). Отдаёт lease «всё ОК» с момента загрузки.
// Заменяет SafetyLink/UartLink; в FIELD не попадает (build_src_filter + nm).

#include "services/safety_source.h"

class NanoStubLink final : public ISafetySource {
public:
    static constexpr const char* kBanner = "NANO STUB - NO SAFETY SUPERVISOR";

    void poll(uint32_t nowMs) override;
    const SafetyLease& lease() const override { return lease_; }

private:
    SafetyLease lease_{};
};

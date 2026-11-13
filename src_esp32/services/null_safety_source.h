#pragma once
// FIELD-заглушка до M3b: lease всегда невалиден, AUTO недостижим.

#include "services/safety_source.h"

class NullSafetySource final : public ISafetySource {
public:
    void poll(uint32_t nowMs) override;
    const SafetyLease& lease() const override { return lease_; }

private:
    SafetyLease lease_{};
};

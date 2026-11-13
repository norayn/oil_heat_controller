#pragma once
// Источник lease (не HAL-порт, а сервисный интерфейс).
// FIELD — SafetyLink; bench-auto — NanoStubLink; до M3b — NullSafetySource.

#include <cstdint>
#include "domain/safety_types.h"

class ISafetySource {
public:
    virtual ~ISafetySource() = default;
    virtual void poll(uint32_t nowMs) = 0;
    virtual const SafetyLease& lease() const = 0;
};

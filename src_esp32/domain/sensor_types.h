#pragma once
// Снимок датчиков (process image, ADR-03). Host-clean.
// Критичные Nano-датчики здесь НЕ дублируются: их статус приходит в lease.

#include <cstdint>

enum class Quality : uint8_t {
    Ok = 0,
    Stale,
    Invalid,
    NotFitted,
    NotUsedInProfile
};

template <typename T>
struct SensorReading {
    T        value{};
    Quality  quality{Quality::Invalid};   // безопасный дефолт
    uint32_t stampMs{0};

    bool usable() const { return quality == Quality::Ok; }
};

enum class FlameStatus : uint8_t { Invalid = 0, Off, On, Degraded };

struct SensorSnapshot {
    SensorReading<float> oilTempMain;
    SensorReading<float> boilerIn;
    SensorReading<float> boilerOut;
    SensorReading<float> boilerCore;
    SensorReading<float> taIn;
    SensorReading<float> taOut;
    SensorReading<float> exhaust;                      // MAX6675, BLK-003
    FlameStatus          flame{FlameStatus::Invalid};  // BLK-005
};

constexpr const char* qualityName(Quality quality) {
    switch (quality) {
        case Quality::Ok:               return "Ok";
        case Quality::Stale:            return "Stale";
        case Quality::Invalid:          return "Invalid";
        case Quality::NotFitted:        return "NotFitted";
        case Quality::NotUsedInProfile: return "NotUsedInProfile";
    }
    return "UNKNOWN";
}

constexpr const char* flameStatusName(FlameStatus status) {
    switch (status) {
        case FlameStatus::Invalid:  return "Invalid";
        case FlameStatus::Off:      return "Off";
        case FlameStatus::On:       return "On";
        case FlameStatus::Degraded: return "Degraded";
    }
    return "UNKNOWN";
}

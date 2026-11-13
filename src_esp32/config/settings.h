#pragma once
// Текущие настройки (ADR-25). В M0 — каркас с дефолтами из limits.h.
// M6 добавит NVS, версию/CRC32, журнал изменений и валидацию all-or-nothing.
//
// Важно: Settings меняет только числа, которые использует FSM.
// Маски, permit, latch и логику SafetySupervisor настройки не трогают.

#include <cstdint>
#include "config/limits.h"

struct Settings {
    // Тайминги горелки
    uint32_t prePurgeMs{kPrePurgeMs};
    uint32_t ignitionMs{kIgnitionMs};
    uint32_t ignitionOilDelayMs{kIgnitionOilDelayMs};
    uint32_t postPurgeMs{kPostPurgeMs};
    uint32_t postPurgeFailMs{kPostPurgeFailMs};
    uint32_t runMaxMs{kRunMaxMs};
    uint32_t stableMs{kStableMs};
    uint32_t flameStableMs{kFlameStableMs};
    uint32_t latchMinMs{kLatchMinMs};

    // Лимиты розжига
    uint8_t  maxIgnitionAttempts{kMaxIgnitionAttempts};
    uint16_t maxIgnitionsPerHour{kMaxIgnitionsPerHour};

    // Температуры и уставки
    float tMaxHeatC{kTmaxHeatC};
    float tMaxTransferC{kTmaxTransferC};
    float taTargetC{60.0f};
    float boilerHysteresisC{5.0f};

    // Подтверждение fan-only продувки (на котле)
    bool purgePathApproved{kPurgePathApproved};
};

inline constexpr Settings kDefaultSettings{};

static_assert(kDefaultSettings.postPurgeFailMs >= kDefaultSettings.postPurgeMs,
              "дефолты: TpostpurgeFail >= Tpostpurge");
static_assert(kDefaultSettings.tMaxHeatC < kDefaultSettings.tMaxTransferC,
              "дефолты: TmaxHeat < TmaxTransfer");
static_assert(kDefaultSettings.latchMinMs > 0,
              "дефолты: TlatchMin > 0");

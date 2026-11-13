#pragma once
// Заводские дефолты и жёсткие границы (ADR-25).
// Здесь только compile-time значения: safety-тайминги, лимиты, периоды задач.
// Текущие значения (в том числе защитные) живут в Settings + NVS и правятся
// через веб с паролем (AR-07). Этот файл — источник дефолтов и диапазонов.

#include <cstdint>
#include "build_profile.h"   // OILCTRL_HAVE_* (обычно уже подключён forced include)

// --- Периоды задач планировщика, мс (§6) -----------------------------------
constexpr uint32_t kControlPeriodMs      = 20;
constexpr uint32_t kFastSensorPeriodMs   = 20;
constexpr uint32_t kSlowSensorPeriodMs   = 200;
constexpr uint32_t kI2cHealthPeriodMs    = 100;
constexpr uint32_t kSafetyLinkPeriodMs   = 100;
constexpr uint32_t kManualMonitorPeriodMs = 500;
constexpr uint32_t kManualConsolePeriodMs = 50;
constexpr uint32_t kHousekeepingPeriodMs = 1000;

// --- Тайминги горелки, мс ---------------------------------------------------
constexpr uint32_t kPrePurgeMs       = 20000;
constexpr uint32_t kIgnitionMs       = 15000;
constexpr uint32_t kIgnitionOilDelayMs = 3000;
constexpr uint32_t kPostPurgeMs      = 30000;
constexpr uint32_t kPostPurgeFailMs  = kPostPurgeMs + 30000;
constexpr uint32_t kRunMaxMs         = 90u * 60u * 1000u;   // 90 мин
constexpr uint32_t kStableMs         = 60000;
constexpr uint32_t kFlameStableMs    = 3000;
constexpr uint32_t kLatchMinMs       = 10000;

// --- Лимиты розжига (§7.5) --------------------------------------------------
constexpr uint8_t  kMaxIgnitionAttempts = 1;    // retry выключен до проверки на котле
constexpr uint16_t kMaxIgnitionsPerHour = 10;

// --- Lease от Nano (§7.3) ---------------------------------------------------
constexpr uint32_t kLeaseTimeoutMs = 600;       // <= 3 периодов heartbeat

// --- Продувка ---------------------------------------------------------------
// false — fan-only продувка ещё не подтверждена на котле.
// Пока false, переход PrePurge -> Ignition невозможен (fail-safe).
constexpr bool kPurgePathApproved = false;

// --- Обратные связи безопасности -------------------------------------------
// Значения приходят из build_flags (OILCTRL_REQUIRE_*), проверенных
// include/build_profile.h. false допустим только до подключения обратной связи;
// в сборке OILCTRL_ENABLE_AUTO=1 оба обязаны быть true (#error в build_profile).
#if OILCTRL_HAVE_CONTACTOR_FB
constexpr bool kRequireContactorFb = true;
#else
constexpr bool kRequireContactorFb = false;
#endif

#if OILCTRL_HAVE_KC868_POWER_FB
constexpr bool kRequireKc868PowerFb = true;
#else
constexpr bool kRequireKc868PowerFb = false;
#endif

// --- Температуры (уставки/границы), °C -------------------------------------
constexpr float kTmaxHeatC     = 80.0f;
constexpr float kTmaxTransferC = 95.0f;

// --- Ручной режим (§8.1) ----------------------------------------------------
constexpr uint32_t kManualDeadmanMs        = 10000;
constexpr uint32_t kManualMaxOnFanMs       = 30u * 60u * 1000u;
constexpr uint32_t kManualMaxOnValveMs     = kRunMaxMs + 10000;
constexpr uint32_t kManualMaxOnIgnitionMs  = kIgnitionMs + 2000;
constexpr uint32_t kManualMaxOnHeaterMs    = 2u * 60u * 60u * 1000u + 30000u;
constexpr uint32_t kManualMaxOnPumpTransferMs = kManualMaxOnValveMs;
constexpr uint32_t kManualMaxOnPumpBoilerMs   = 30u * 60u * 1000u;

constexpr uint32_t kManualMinOffFanMs          = 0;
constexpr uint32_t kManualMinOffValveMs        = 1000;
constexpr uint32_t kManualMinOffIgnitionMs     = 5000;
constexpr uint32_t kManualMinOffHeaterMs       = 30000;
constexpr uint32_t kManualMinOffPumpTransferMs = 1000;
constexpr uint32_t kManualMinOffPumpBoilerMs   = 1000;

// --- Watchdog ---------------------------------------------------------------
constexpr uint32_t kWdtTimeoutMs = 1500;

// --- Обязательные взаимные ограничения (compile-time) ---------------------
static_assert(kPrePurgeMs > 0, "Tprepurge должен быть > 0");
static_assert(kPostPurgeMs > 0, "Tpostpurge должен быть > 0");
static_assert(kPostPurgeFailMs >= kPostPurgeMs,
              "TpostpurgeFail обязан быть не меньше Tpostpurge");
static_assert(kIgnitionOilDelayMs + 1000 < kIgnitionMs,
              "TignOilDelay + margin должен быть меньше Tign");
static_assert(kRunMaxMs > kStableMs + 10000,
              "TrunMax должен быть больше Tstable + margin");
static_assert(kLatchMinMs > 0, "TlatchMin должен быть > 0");
static_assert(kTmaxHeatC < kTmaxTransferC,
              "TmaxHeat должен быть меньше TmaxTransfer");
static_assert(kMaxIgnitionAttempts >= 1, "должна быть хотя бы одна попытка розжига");
static_assert(kMaxIgnitionsPerHour > 0, "rate-limit розжига должен быть > 0");
static_assert(kManualDeadmanMs > 0 && kManualDeadmanMs <= 10000,
              "deadman опасной группы: > 0 и <= 10 000 мс");
static_assert(kManualMaxOnIgnitionMs >= kIgnitionMs + 2000,
              "FIELD_CEILING(ignition) >= Tign + 2 c");
static_assert(kManualMaxOnValveMs >= kRunMaxMs + 10000,
              "FIELD_CEILING(valves) >= TrunMax + 10 c");
static_assert(kManualMaxOnHeaterMs >= 2u * 60u * 60u * 1000u,
              "FIELD_CEILING(heater) >= 2 ч");
static_assert(kManualMaxOnValveMs >= kManualMaxOnIgnitionMs,
              "max-on клапанов не может быть меньше max-on розжига");

#pragma once
// Матрица допустимых комбинаций сборок.
// Подключается forced include (-include) во всех env: ESP32, AVR8 и host.
//
// Здесь макросы ТОЛЬКО проверяются. Определять OILCTRL_TARGET_* и
// OILCTRL_ENABLE_* в коде запрещено — только через build_flags env.

// --- цели сборки -----------------------------------------------------------
#if defined(OILCTRL_TARGET_ESP32)
#  define OILCTRL_IS_ESP32 1
#else
#  define OILCTRL_IS_ESP32 0
#endif

#if defined(OILCTRL_TARGET_ESP32_MANUAL)
#  define OILCTRL_IS_ESP32_MANUAL 1
#else
#  define OILCTRL_IS_ESP32_MANUAL 0
#endif

#if defined(OILCTRL_TARGET_ESP32_BENCH_AUTO)
#  define OILCTRL_IS_ESP32_BENCH_AUTO 1
#else
#  define OILCTRL_IS_ESP32_BENCH_AUTO 0
#endif

#if defined(OILCTRL_TARGET_AVR8)
#  define OILCTRL_IS_AVR8 1
#else
#  define OILCTRL_IS_AVR8 0
#endif

#if defined(OILCTRL_TARGET_HOST)
#  define OILCTRL_IS_HOST 1
#else
#  define OILCTRL_IS_HOST 0
#endif

#define OILCTRL_TARGET_COUNT \
    (OILCTRL_IS_ESP32 + OILCTRL_IS_ESP32_MANUAL + OILCTRL_IS_ESP32_BENCH_AUTO + \
     OILCTRL_IS_AVR8 + OILCTRL_IS_HOST)

#if OILCTRL_TARGET_COUNT > 1
#  error "Задано больше одной цели сборки (OILCTRL_TARGET_*)"
#endif

// --- флаги режимов ---------------------------------------------------------
// Флаги допустимы только со значением 1; «0» не используется, чтобы не
// появлялось двух способов выключить защиту.
#if defined(OILCTRL_ENABLE_MANUAL_BYPASS)
#  if OILCTRL_ENABLE_MANUAL_BYPASS != 1
#    error "OILCTRL_ENABLE_MANUAL_BYPASS допускается только =1"
#  endif
#  define OILCTRL_IS_MANUAL 1
#else
#  define OILCTRL_IS_MANUAL 0
#endif

#if defined(OILCTRL_ENABLE_NANO_STUB)
#  if OILCTRL_ENABLE_NANO_STUB != 1
#    error "OILCTRL_ENABLE_NANO_STUB допускается только =1"
#  endif
#  define OILCTRL_IS_NANO_STUB 1
#else
#  define OILCTRL_IS_NANO_STUB 0
#endif

#if defined(OILCTRL_ENABLE_AUTO)
#  if OILCTRL_ENABLE_AUTO != 1
#    error "OILCTRL_ENABLE_AUTO допускается только =1"
#  endif
#  define OILCTRL_IS_AUTO 1
#else
#  define OILCTRL_IS_AUTO 0
#endif

#if defined(OILCTRL_REQUIRE_CONTACTOR_FB)
#  if OILCTRL_REQUIRE_CONTACTOR_FB != 1
#    error "OILCTRL_REQUIRE_CONTACTOR_FB допускается только =1"
#  endif
#  define OILCTRL_HAVE_CONTACTOR_FB 1
#else
#  define OILCTRL_HAVE_CONTACTOR_FB 0
#endif

#if defined(OILCTRL_REQUIRE_KC868_POWER_FB)
#  if OILCTRL_REQUIRE_KC868_POWER_FB != 1
#    error "OILCTRL_REQUIRE_KC868_POWER_FB допускается только =1"
#  endif
#  define OILCTRL_HAVE_KC868_POWER_FB 1
#else
#  define OILCTRL_HAVE_KC868_POWER_FB 0
#endif

// --- несовместимые комбинации ---------------------------------------------
#if OILCTRL_IS_AVR8 && OILCTRL_TARGET_COUNT > 1
#  error "Цель AVR8 несовместима с любой другой целью"
#endif

#if OILCTRL_IS_ESP32_MANUAL && !OILCTRL_IS_MANUAL
#  error "TARGET_ESP32_MANUAL требует OILCTRL_ENABLE_MANUAL_BYPASS=1"
#endif

#if OILCTRL_IS_MANUAL && !(OILCTRL_IS_ESP32_MANUAL || OILCTRL_IS_HOST)
#  error "OILCTRL_ENABLE_MANUAL_BYPASS допустим только в manual-сборке"
#endif

#if OILCTRL_IS_ESP32_BENCH_AUTO && !OILCTRL_IS_NANO_STUB
#  error "TARGET_ESP32_BENCH_AUTO требует OILCTRL_ENABLE_NANO_STUB=1"
#endif

#if OILCTRL_IS_NANO_STUB && !(OILCTRL_IS_ESP32_BENCH_AUTO || OILCTRL_IS_HOST)
#  error "OILCTRL_ENABLE_NANO_STUB допустим только в bench-auto-сборке"
#endif

#if OILCTRL_IS_MANUAL && OILCTRL_IS_NANO_STUB
#  error "manual и bench-auto взаимоисключающие"
#endif

#if OILCTRL_IS_ESP32 && OILCTRL_IS_MANUAL
#  error "Полевая сборка ESP32 не может содержать manual-режим"
#endif

#if OILCTRL_IS_ESP32 && OILCTRL_IS_NANO_STUB
#  error "Полевая сборка ESP32 не может содержать заглушку Nano"
#endif

#if OILCTRL_IS_ESP32_BENCH_AUTO && !OILCTRL_IS_AUTO
#  error "bench-auto — это автоматический режим с другим источником lease"
#endif

#if OILCTRL_IS_MANUAL && OILCTRL_IS_AUTO
#  error "manual-сборка не содержит автоматического режима"
#endif

#if OILCTRL_IS_AUTO && !OILCTRL_HAVE_CONTACTOR_FB
#  error "Автоматический режим требует OILCTRL_REQUIRE_CONTACTOR_FB=1"
#endif

#if OILCTRL_IS_AUTO && !OILCTRL_HAVE_KC868_POWER_FB
#  error "Автоматический режим требует OILCTRL_REQUIRE_KC868_POWER_FB=1"
#endif

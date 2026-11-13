# Архитектура кода Main Unit (ESP32 / KC868-A6) — v0.9

Статус: **v0.9 — рабочий документ, обязательных гейтов нет.** Безопасность обеспечивает **финальная полевая сборка** (`esp32_core` + Nano). Отладочные сборки защит не содержат; принятые риски записаны в `docs/safety/accepted_risks.md` и не превращаются в блокеры.
Дата: 2026-11-13 (v0.9).
Область: `src_esp32/`, `platformio.ini` (предлагаемые env), границы с `src_avr8/` и `include/common_protocol.h`.
Связанные документы: `docs/architecture/modules.md`, `docs/hardware/periph_distribution.md`, `docs/hardware/pinmap.md`, `docs/protocol/uart.md`, `docs/architecture/blockers.md`, `docs/safety/fsm_safety_boundary.md`, `docs/safety/checklist.md`.

> **Главный принцип документа:** fail-safe обязателен только в **полевой** прошивке. Отладочные сборки (manual, bench-auto) защит не содержат: это их назначение, а не дефект. Изоляция сборок на уровне `build_src_filter` остаётся — она бесплатна и гарантирует, что отладочный код не попадёт в FIELD.
>
> Обязательных ревью, актов и журналов нет. Если риск принят — он в `docs/safety/accepted_risks.md`, и на этом всё.
>
> **Отладка идёт на котле.** Отдельного стенда, эквивалентов нагрузки и маркировки платы нет. Отладочная сборка может быть прошита на ту же плату и остаться там по забывчивости — это AR-01/AR-02 в риск-регистре.

## 0. История версий

**v0.9 — упрощение (2026-11-13):**

| Что изменилось | Следствие |
|---|---|
| Блокеры и обязательные ревью сняты | `blockers.md` — справочный список; риски — `docs/safety/accepted_risks.md` |
| Отладочные сборки без барьеров | нет `CONFIRM`, runtime-бюджета, UART-probe, маркировки, процедуры, журнала (AR-02…AR-04) |
| Отладка на котле | нет стенда, эквивалентов нагрузки, «bench-only» исключений (AR-01) |
| Защитные константы — runtime | правятся через веб с паролем; класса «только пересборкой» больше нет (AR-07) |
| ТЭН в горении без гейта | `kHeaterInRunApproved` удалён; остаётся `permitHeater && canHeatOil` (AR-06) |
| История ревью v0.1–v0.8 удалена | она описывала процесс с обязательными согласованиями, которого больше нет |

**Что пришло из v0.8 и остаётся:** ТЭН разрешён в `Idle` и `Run`; идентификатора платы нет; есть третья сборка bench-auto; все настройки в одном месте.

**Что было раньше (кратко):** v0.1–v0.7 — поэтапная проработка домена, единого писателя актуаторов, lease/supervisor, изоляции manual-сборки и retry allow-list. Спорные вопросы тех версий либо решены в коде, либо переведены в §16.

---

## 1. Цели и границы

### 1.1. Что должна дать архитектура

1. **Быстрый старт шага 4**: ручное управление котлом на реальной установке без ожидания полного safety-стека.
2. **Безболезненный рост** до автоматического FSM, связи с Nano, теплоаккумулятора, RS485 и тёплых полов.
3. **Детерминизм и проверяемость**: чистая доменная логика, тестируемая на хосте.
4. **Fail-safe по умолчанию** в FIELD; осознанный bypass — только в manual-сборке.
5. **Один писатель выходов** — исключить конфликт «FSM vs ручная команда vs будущий RS485».

### 1.2. Что НЕ входит в этот документ

- Физические пины, полярности, номиналы, схема 220 В (→ шаг 3, `pinmap.md`).
- Формат/CRC/таймауты safety UART (→ BLK-006, `uart.md`, `common_protocol.h`).
- Реализация протокола и любые правки `include/common_protocol.h`.
- Схемотехника Nano (cutoff, feedback, power-cycle).
- PID ТЭНа, погодозависимость, Wi-Fi/веб, RS485-протокол (только интерфейсы-заглушки).

### 1.3. Приоритет документов

При конфликте: `docs/architecture/modules.md`, `docs/architecture/blockers.md`, `docs/hardware/periph_distribution.md`, `docs/protocol/uart.md` — **выше** этого документа. Здесь фиксируется только структура кода.

---

## 2. Ключевые архитектурные решения (ADR)

| ID | Решение | Обоснование |
|---|---|---|
| ADR-01 | **Модульный монолит, один управляющий поток** (cooperative superloop) | Детерминизм, отсутствие гонок. Wi-Fi/RS485 позже — только через очереди, FSM не трогают. |
| ADR-02 | **Слои `domain → services → app`**, HAL реализует порты; `domain` не знает Arduino | Домен тестируется на хосте, железо заменяется фейками. |
| ADR-03 | **Process image**: один снимок состояния на тик | Явные входы/выходы FSM, воспроизводимость, тесты. |
| ADR-04 | **Единственный писатель актуаторов** — `ActuatorService` | Невозможен конфликт команд; централизованные safe-маски. |
| ADR-05 | **Три сборки: `FIELD`, подмножество FIELD + `OILCTRL_ENABLE_MANUAL_BYPASS` (manual), FIELD-ядро + `OILCTRL_ENABLE_NANO_STUB` (bench-auto)**; bench-код физически исключён из FIELD (`build_src_filter`, отдельные каталоги `manual/`, `bench/`) | Режимы без защит отсутствуют в полевом бинарнике. «Подмножество» = без supervisor/link/FSM/interlocks/retry. Отдельного «профиля» в смысле v0.4 нет. |
| ADR-06 | **Разрешение — явный lease от Nano** (`valid + permit + gate + latch + alarms + sensorValid`), default deny | Нельзя отличить «жив, но запрещено» от «разрешено» без явного поля (`uart.md`). |
| ADR-07 | **Время только через `IClock`**, никаких `delay()` в рабочем цикле | Тестируемость, WDT, отсутствие блокировок. |
| ADR-08 | **Статические структуры, без динамической памяти после init** | Предсказуемость, отсутствие heap-отказов. |
| ADR-09 | **Latch — липкий с M4**; reboot не снимает аварию (с M4; до M4 AUTO недостижим) | Запрет авто-возобновления опасных режимов. |
| ADR-10 | **Никогда не было разрешения ≠ потеря разрешения** | Ранние FIELD-сборки стоят в `Off/Deny`; потеря после валидного lease — `Latch`. |
| ADR-11 | **Domain не содержит profile-веток**: `RunMode::Manual` присутствует в enum всегда, гейтится только инстанцирование в `app/` | Один layout/ODR для всех сборок; host-тесты видят полный enum |
| ADR-12 | **Manual Bypass включается только дефайном** `OILCTRL_ENABLE_MANUAL_BYPASS` из `build_flags`; аппаратного ключа и привязки к плате нет | Простое переключение сборки; риск закрывается `#error`/CI/`nm`, runtime-барьерами (§10.2) и **процедурой перепрошивки FIELD перед установкой на котёл**. Остаточный риск — §17. |
| ADR-13 | **Reboot/power-on не возобновляет опасный режим** | После reset — `Off`, manual не персистится |
| ADR-14 | **Safety-лимиты не runtime-tunable**; настройки эксплуатации — runtime | Защиту нельзя «подкрутить» из NVS/веб; уставки, калибровки и расписания — можно, с валидацией (§9.3) |
| ADR-15 | **Единый арбитр безопасности** — `SafetySupervisor` для AUTO | Нет расхождения «FSM в Run, актуаторы уже latch» |
| ADR-16 | **Gate handshake без цикла:** `Idle→PrePurge` — `permitProcess`; `PrePurge→Ignition` — `permitDangerous` | Nano не зависит от `fsmState` для открытия gate |
| ADR-17 | **Feedback handshake без цикла:** Nano замыкает контактор независимо от Main; `permit → ON → ON_CONFIRMED → permitDangerous` | Иначе розжиг недостижим |
| ADR-18 | **Manual Bypass вне пути `SafetySupervisor`**: supervisor не инстанцируется в manual-сборке; команды не проходят через permit/interlocks | Прямое управление для отладки; FIELD не затрагивается |
| ADR-19 | **Повторный розжиг — allow-list**: разрешён только для явно перечисленных причин, всё остальное (включая `None` и будущие) — запрещено | Fail-safe: неизвестная причина не может разрешить повторный розжиг |
| ADR-20 | **В manual-режиме мониторинг датчиков обязателен**, но не блокирует команды | Оператор видит реальное состояние; мониторинг ≠ interlock |
| ADR-21 | **Состояние FSM задаёт `demand`**: `Fault`/`PostPurge`/`Shutdown`/`Latched` ⇒ `kSafetyOffGroup` OFF независимо от lease | `permitDangerous`/`permitHeater` необходимы, но недостаточны; приоритет — у безопасного состояния |
| ADR-22 | **В manual Nano физически отключён и обесточен** | Иначе SR-B6 power-cycle → POR PCF8574 → импульсная активация реле |
| ADR-23 | **Барьеров в manual нет** (заменено в v0.9): ни runtime-бюджета, ни UART-probe, ни `CONFIRM`, ни маркировки | Риски AR-02…AR-04 приняты; остались только max-on/deadman и проверка «все выходы OFF» перед первой командой |
| ADR-24 | **bench-auto — автоматика с заглушкой Nano**: `OILCTRL_ENABLE_NANO_STUB`, отдельная сборка, заглушка сразу отдаёт lease «всё ОК» | Отладка FSM без Nano; заглушка не попадает в FIELD; риск AR-05 |
| ADR-25 | **Единый центр настроек** `config/`: все числа (включая защитные тайминги) — в `Settings`, NVS + веб с паролем; `limits.h` — только дефолты и диапазоны | Одно место правки; подбор таймингов без пересборки; риск AR-07 |
| ADR-26 | **ТЭН масла разрешён в `Idle` и `Run`** — подогрев нужен и при горении, иначе холодное масло забивает тракт; условие одно: `permitHeater && canHeatOil` | Без compile-time гейта (AR-06); в `Ignition`/продувке/fault — OFF; внешний термостат и термопредохранитель — обязательное железо финальной сборки |
| ADR-27 | **Привязки к плате нет** (`BOARD_ID`/`BENCH_ID` отсутствуют): отладочная прошивка ставится на ту же плату, что поедет в котёл | Одна плата на всё; компенсаций нет, риск AR-02 принят. Видно только по баннеру |

---

## 3. Слои и зависимости

```
                    ┌──────────────────────────────────────────┐
                    │ app/  (composition root, wiring, config) │
                    │  main.cpp, app_common/field/manual       │
                    └───────────────┬──────────────────────────┘
                                    │ создаёт и связывает
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
┌───────▼─────────┐      ┌──────────▼──────────┐      ┌─────────▼─────────┐
│ services/       │      │ domain/             │      │ hal/ + platform/  │
│ SensorService   │─────▶│ BurnerFsm           │◀─────│ ESP32 GPIO        │
│ ActuatorService │      │ Interlocks          │      │ PCF8574 relays    │
│ SafetyLink      │      │ SafetySupervisor    │      │ DS18B20 / MAX6675 │
│ ManualControl*  │      │ RetryPolicy         │      │ UART link         │
│ ManualConsole*  │      │ ModeManager         │      │ Clock / WDT / NVS │
│ ManualMonitor*  │      │ SensorTypes/Faults  │      │ Host fakes (test) │
└─────────────────┘      └─────────────────────┘      └───────────────────┘
        * только manual-сборка; код в отдельном каталоге src_esp32/manual/
```

**Правила зависимостей (ревью/CI):**

- `domain/` — чистый C++: без `<Arduino.h>`, без `hal/`, без `services/`, без глобального времени, **без `#if OILCTRL_*`**.
- `services/` — оркестрация, знает `domain/` и **интерфейсы** HAL (`ports`); общие сервисы не знают о manual.
- `manual/` — единственное место manual-логики (control/console/monitor/manual-порт актуаторов); зависит от `services/` и `domain/`, но **никто из `services/`/`domain/` не зависит от `manual/`**.
- `bench/` — единственное место заглушки Nano (bench-auto); зависит от `services/` и `domain/`, но **никто из `services/`/`domain/` не зависит от `bench/`**. Подставляется только в `app_bench_auto.cpp` через интерфейс `ISafetySource`.
- `hal/` — только драйверы, без прикладной логики.
- `app/` — единственное место создания объектов и выбора сборки.
- Никто, кроме `ActuatorService`, не пишет в выходы.
- Никто, кроме `SafetyLink`, не читает safety UART; **исключение:** `manual/uart_probe` — только в manual-сборке, read-only, без TX/парсинга/ответа. Никто, кроме `SensorService`, не читает датчики.
- `ModeManager`, `SafetySupervisor`, `RetryPolicy`, `BurnerFsm` — в `domain/`.
- В manual-сборке `SafetySupervisor`, `SafetyLink`, `BurnerFsm`, `Interlocks`, `RetryPolicy` **не компилируются и не линкуются** (§10.1).

---

## 4. Предлагаемая структура `src_esp32/`

```
src_esp32/
├── main.cpp                     [M0] setup()/loop(): app::setup()/app::tick()
├── app/
│   ├── app.h                    [M0]
│   ├── app_common.cpp           [M0] только profile-agnostic wiring (clock/log/WDT/limits)
│   ├── app_field.cpp            [M0] FIELD composition root
│   ├── app_manual.cpp           [M0] MANUAL composition root (без supervisor/link/FSM)
│   ├── app_bench_auto.cpp       [M4b] BENCH-AUTO composition root (FSM+supervisor+заглушка lease)
│   ├── null_manual_request.h    [M0] NullManualRequest (всегда false) — живёт в app/
│   └── process_image.h          [M2] снимок входов тика (§5)
├── config/                      ЕДИНСТВЕННОЕ место настроек (ADR-25)
│   ├── limits.h                 [M0] safety-тайминги/лимиты, static_assert (compile-time, не из веба)
│   ├── settings.h               [M0] каркас; [M6] runtime-настройки: структура, границы, дефолты
│   └── settings_validate.cpp    [M6] диапазоны и взаимные ограничения
├── domain/
│   ├── actuator_types.h         [M0] ActuatorId + маски
│   ├── faults.h                 [M0]
│   ├── mode_manager.h/.cpp      [M0] BootSafe/Off/Manual/Auto/Fault/Latched
│   ├── safety_types.h           [M0] SafetyLease/SafetyDecision
│   ├── safety_policy.h/.cpp     [M0]
│   ├── safety_supervisor.h/.cpp [M4]
│   ├── retry_policy.h/.cpp      [M4] allow-list повторного розжига
│   ├── burner_types.h           [M4]
│   ├── burner_fsm.h/.cpp        [M4]
│   ├── interlocks.h/.cpp        [M4]
│   ├── sensor_types.h           [M2]
│   └── setpoints.h              [M5]
├── services/
│   ├── scheduler.h/.cpp         [M0]
│   ├── actuator_service.h/.cpp  [M0] две точки входа `tickAuto`/`tickManual` + общий `apply()`; manual — через порт
│   ├── diagnostics.h/.cpp       [M1]
│   ├── sensor_service.h/.cpp    [M2]
│   ├── safety_source.h          [M0] ISafetySource: интерфейс источника lease
│   ├── null_safety_source.h/.cpp [M0] NullSafetyLink: ISafetySource, valid=false
│   ├── safety_link.h/.cpp       [M3b] FIELD-реализация ISafetySource (UART)
│   ├── settings_service.h/.cpp  [M6] NVS + CRC + журнал изменений + snapshot настроек
│   ├── web_config.h/.cpp        [M6] веб-интерфейс настроек (Wi-Fi), отдельная задача
│   └── storage.h/.cpp           [M4]
├── manual/                      [M0] компилируется ТОЛЬКО в manual-сборке
│   ├── manual_control.h/.cpp    enter/exit/set/allOff, max-on/deadman; реализует IManualActuatorPort
│   ├── manual_console.h/.cpp    парсер команд
│   ├── manual_monitor.h/.cpp    периодический лог датчиков (500 мс)
│   ├── manual_actuator_port.h   IManualActuatorPort (реализация — ManualControlService)
│   ├── actuator_service_manual.cpp  ЕДИНСТВЕННОЕ определение ActuatorService::tickManual
│   ├── supervisor_probe.h/.cpp  read-only детект трафика safety UART (перенос из hal/)
├── bench/                       [M4b] компилируется ТОЛЬКО в bench-auto-сборке
│   ├── nano_stub_link.h/.cpp    NanoStubLink: заглушка lease «всё ОК» вместо SafetyLink/UartLink
│   ├── nano_stub_link.h/.cpp    заглушка Nano: lease «всё ОК» сразу после загрузки
│   └── bench_auto_console.h/.cpp  команды подтверждения/выхода из bench-auto
├── hal/
│   ├── ports.h                  [M0] IClock/IDigitalOut/ITempBus/IExhaustTemp/IFlameInput/ILinkTransport/IHeatDemand/IStopInput/IResetInput/IManualRequest/INvs
│   ├── dummy_out.h              [M0]
│   ├── esp32_gpio.h/.cpp        [M1]
│   ├── pcf8574_relay.h/.cpp     [M1]
│   ├── ds18b20_bus.h/.cpp       [M2]
│   ├── max6675.h/.cpp           [M2]
│   ├── uart_link.h/.cpp         [M3a]
│   └── status_led.h/.cpp        [M1]
├── platform/
│   ├── clock.h/.cpp             [M0]
│   ├── log.h/.cpp               [M0]
│   ├── watchdog.h/.cpp          [M0]
│   ├── net.h/.cpp               [M6] Wi-Fi + HTTP-сервер; не блокирует управляющий цикл
│   ├── nvs.h/.cpp               [M0] runtime-настройки; [M4] latch/счётчики
│   ├── early_safe.h/.cpp        [M0] direct GPIO OFF/pull-down до Serial/I2C (§11.1)
│   └── host/
│       ├── clock_host.cpp       [M0]
│       └── log_host.cpp         [M0]
└── (нет tests/ — host-тесты в корневом test/)
```

**Composition:** `app_field.cpp` — FIELD (supervisor+link+FSM); `app_manual.cpp` — manual (sensor+actuator+manual control, **без** supervisor/link/FSM); `app_bench_auto.cpp` — bench-auto (sensor+supervisor+FSM+actuator, источник lease — `NanoStubLink` вместо `SafetyLink`). Взаимоисключающие через `build_src_filter`; manual/bench-код изолирован каталогами `manual/`/`bench/`, а не перечислением файлов. Единственные исключения вне каталогов — `app/app_manual.cpp` и `app/app_bench_auto.cpp` (composition roots); в FIELD-фильтре они исключены явно.

**Поток управления (оба профиля):** `app::tick()` → sensors → (AUTO: safety → FSM) → `ModeManager` → **один** вызов записи в железо: в FIELD `ActuatorService::tickAuto(now, demand)`, в manual `ManualControlService::tick(now)` → `ActuatorService::tickManual(now, bits)`. `ActuatorService` не знает про `manual/`: `tickManual` только объявлен, определён в `manual/`.

**Граница `app_common.cpp`:** только profile-agnostic wiring — `IClock`, `Log`, `Watchdog`, `limits`, `ActuatorService::beginSafe()`. Никаких упоминаний `SafetySupervisor`, `SafetyLink`, `BurnerFsm`, `Manual*`. Нарушение ловится двусторонней `nm`-проверкой (§10.1).

---

## 5. Модель данных (process image)

```cpp
// domain/sensor_types.h
enum class Quality : uint8_t { Ok, Stale, Invalid, NotFitted, NotUsedInProfile };

template <typename T>
struct SensorReading {
    T        value{};
    Quality  quality{Quality::Invalid};   // безопасный дефолт
    uint32_t stampMs{0};
    bool usable() const { return quality == Quality::Ok; }
};

enum class FlameStatus : uint8_t { Invalid, Off, On, Degraded };

struct SensorSnapshot {
    SensorReading<float> oilTempMain;
    SensorReading<float> boilerIn;
    SensorReading<float> boilerOut;
    SensorReading<float> boilerCore;
    SensorReading<float> taIn;
    SensorReading<float> taOut;
    SensorReading<float> exhaust;   // MAX6675, BLK-003
    FlameStatus          flame{FlameStatus::Invalid}; // BLK-005
    // Критичные Nano-датчики не дублируются как источник решения:
    // их статус приходит в lease/permit/sensorValid.
};
```

```cpp
// domain/actuator_types.h
enum class ActuatorId : uint8_t {
    FanBlower = 0, OilValveMain, AirValveMain, IgnitionUnit,
    HeaterOil, PumpOilTransfer, PumpBoiler, Count
};
static_assert(static_cast<uint8_t>(ActuatorId::Count) <= 16);

using ActuatorBits = uint16_t;
constexpr ActuatorBits bit(ActuatorId id) { return uint16_t(1u) << uint8_t(id); }

constexpr ActuatorBits kAllOff        = 0;
constexpr ActuatorBits kCombustion    = bit(ActuatorId::OilValveMain) | bit(ActuatorId::AirValveMain);
constexpr ActuatorBits kIgnitionGroup = kCombustion | bit(ActuatorId::IgnitionUnit);
constexpr ActuatorBits kDangerGroup   = kIgnitionGroup | bit(ActuatorId::PumpOilTransfer);  // !permitDangerous
constexpr ActuatorBits kSafetyOffGroup = kDangerGroup | bit(ActuatorId::HeaterOil);         // fault/latch/all-off
constexpr ActuatorBits kProcessGroup  = bit(ActuatorId::FanBlower) | bit(ActuatorId::PumpBoiler);
static_assert((kCombustion & kIgnitionGroup) == kCombustion);
// ACT_*_VALVE_SAFETY здесь ОТСУТСТВУЮТ by design: ими управляет только Nano.
```

**Per-bit поведение в AUTO при `!permitDangerous`:**

| Бит | Поведение | Причина |
|---|---|---|
| `OilValveMain`, `AirValveMain`, `IgnitionUnit`, `PumpOilTransfer` | **немедленно OFF** при `!permitDangerous` | опасная группа |
| `HeaterOil` | **немедленно OFF** при `!permitHeater` (не при `!permitDangerous`) | ТЭН, не топливо: нужен контактор, `kValidOilTempSafety` и `kValidFloatMin`, но не открытие safety-gate |
| `FanBlower` | может остаться ON только в `PrePurge`/`PostPurge`/`Fault` | продувка |
| `PumpBoiler` | по SR-B17; по умолчанию OFF, пока не проверено на котле | циркуляция |

В **Manual Bypass** маска `permitDangerous` не применяется: команды прямые. Но `ACT_*_VALVE_SAFETY` по-прежнему недоступны.

### 5.1. Инвариант «состояние → demand» (AUTO)

`permitDangerous` — **необходимое, но не достаточное** условие для опасной группы. Итоговый `demand` определяется **состоянием FSM**, и безопасное состояние имеет приоритет на выключение:

| `BurnerState` | `kFuelGroup`+`kSparkGroup` | `HeaterOil` | `AirValveMain` | `FanBlower` | `PumpBoiler` |
|---|---|---|---|---|---|
| `Init`, `Latched`, `Shutdown` | **OFF** | **OFF** | OFF | OFF | OFF (BLK-007) |
| `Idle` | **OFF** | `permitHeater && canHeatOil` | OFF | OFF | OFF |
| `PrePurge` | **OFF** | **OFF** | **OFF** | ON | по BLK-007 |
| `Ignition` | spark + oil (после `TignOilDelay`) | **OFF** | OPEN (распыление) | ON | по BLK-007 |
| `Run` | oil OPEN, spark OFF | `permitHeater && canHeatOil` | OPEN (распыление) | ON | по BLK-007 |
| `PostPurge` | **OFF** | **OFF** | **OFF** | ON | по BLK-007 |
| `Fault` | **OFF** | **OFF** | **OFF** | ON (постпродувка) | OFF (BLK-007) |

**Продувка — только вентилятором.** `AirValveMain` открыт **исключительно** в `Ignition`/`Run`, где он подаёт сжатый воздух на распыление (`rnd/peripherals.md`). В `PrePurge`/`PostPurge`/`Fault` он **всегда OFF**: по назначению клапан не участвует в продувке, и открывать его «на всякий случай» нельзя. Рабочий воздух в `Ignition`/`Run` не гейтится `kPurgePathApproved` — без него невозможно горение; он разрешён только при `permitDangerous`.

`kPurgePathApproved` — **не разрешение открывать клапан**, а подтверждение того, что **fan-only** продувка достаточна (объём/время/герметичность). Он гейтит **ровно один** переход — `PrePurge → Ignition`.

`HeaterOil` — **отдельная группа** `kHeaterGroup`, не `kFuelGroup`: это ТЭН подогрева масла, а не подача топлива. ТЭН разрешён в **`Idle` (подогрев перед пуском) и `Run` (подогрев во время горения)**: без подогрева в работе холодное масло загустевает и забивает топливный тракт. В обоих состояниях действует одно условие — `permitHeater && canHeatOil`. Программного гейта «после монтажа термостата» нет (AR-06): термостат и термопредохранитель — обязательное железо финальной сборки, но код их наличие не проверяет. В `Ignition` (короткий переход), `PrePurge`/`PostPurge`/`Fault`/`Latched`/`Shutdown` — **OFF**: там либо идёт продувка, либо опасная группа снята.

Подогрев в `Run` **не** является независимой тепловой защитой: `HeaterOil` остаётся под `permitHeater` (контактор, питание платы, `kValidOilTempSafety`, `kValidFloatMin`), под внешним термостатом/термопредохранителем (M1b) и внутри `kSafetyOffGroup` — любой fault снимает его вместе с топливом. Отдельного аппаратного ограничителя температуры ТЭН на Main Unit нет (§17).

`PumpOilTransfer` остаётся в `kFuelGroup`: подкачка в `Idle` **намеренно не выполняется** в M0–M4; расширение — после проверки контактора и поплавков на котле (подкачка — топливная группа под Nano-контактором).

Маски:

```cpp
constexpr ActuatorBits kFuelGroup  = bit(ActuatorId::OilValveMain)
                                   | bit(ActuatorId::PumpOilTransfer);
constexpr ActuatorBits kSparkGroup = bit(ActuatorId::IgnitionUnit);
constexpr ActuatorBits kHeaterGroup = bit(ActuatorId::HeaterOil);   // ТЭН, не топливо
constexpr ActuatorBits kAirPath    = bit(ActuatorId::AirValveMain);

```

Формальный инвариант проверяется **по `decision.next`** (целевому состоянию) и только для AUTO:

```
state ∈ {Fault, PostPurge, Shutdown, Latched}  ⇒  (demand & kSafetyOffGroup) == 0
state == Init                                  ⇒  demand == kAllOff
state == Idle                                  ⇒  (demand & (kFuelGroup|kSparkGroup|kAirPath)) == 0
                                                  && (HeaterOil ⇒ (permitHeater && allowHeatOil))
state == Run                                   ⇒  (demand & kFuelGroup) == bit(ActuatorId::OilValveMain)
                                                  && (demand & kSparkGroup) == 0
                                                  && (demand & (kAirPath | bit(ActuatorId::FanBlower)))
                                                       == (kAirPath | bit(ActuatorId::FanBlower))
                                                  && (HeaterOil ⇒ (permitHeater && allowHeatOil))
state == PrePurge                              ⇒  (demand & kSafetyOffGroup) == 0
```

`kSafetyOffGroup` включает `OilValveMain`, `AirValveMain`, `IgnitionUnit`, `PumpOilTransfer`, `HeaterOil`. `allowHeatOil = canHeatOil(...).allow` вычисляется **до** применения: в `Idle` ТЭН — единственный разрешённый «силовой» актуатор, в `Run` он добавляется к работающему горению; в обоих случаях — только по этому флагу вместе с `permitHeater`.

Исключений «air открыт в продувке» **нет**: инвариант для `PrePurge`/`PostPurge`/`Fault` — тот же `kSafetyOffGroup`, и `AirValveMain` в нём присутствует. Ранее предлагавшееся условие `kPurgePathApproved && FanBlower == ON` удалено как противоречащее назначению клапана (распыление) и собственному инварианту.

`kPurgePathApproved` — `constexpr=false`, пока достаточность **fan-only** продувки не проверена на котле. **Пока флаг `false`, `PrePurge → Ignition` невозможен** — намеренный fail-safe: розжиг после неподтверждённой продувки не допускается.

Если `demand` нарушает инвариант — соответствующие биты **принудительно снимаются**, поднимается `Fault(Internal)`; «лишний» бит никогда не применяется. Это не заменяет Nano, но исключает «долив топлива в горячую камеру + повторный розжиг» при retry.

**Точка проверки:** `ActuatorDemand` несёт состояние и разрешения; manual его не использует (идёт через `tickManual`):

```cpp
struct ActuatorDemand {
    ActuatorBits bits{kAllOff};
    BurnerState  state{BurnerState::Idle};
    bool         permitDangerous{false}; // fail-closed: без явного разрешения опасная группа OFF
    bool         permitHeater{false};    // ТЭН: отдельное разрешение, без gate OPEN_SAFE
    bool         allowHeatOil{false};    // canHeatOil: ТЭН в Idle и Run, по датчикам
};
```

`ActuatorService::tickAuto(now, demand)` **всегда** применяет обе маски: `!permitDangerous → kDangerGroup OFF`, `!permitHeater → kHeaterGroup OFF`. **Runtime-переключателя режима нет** (см. §8.1): manual использует отдельную точку `tickManual`, определение которой физически отсутствует в FIELD.

Все разрешения — fail-closed дефолты: `ActuatorDemand d;` даёт `bits=kAllOff`, `permitDangerous=false`, `permitHeater=false`, `allowHeatOil=false`, то есть маски §5.1 **включаются**, а не выключаются. Признака AUTO/`autoMode` в типе нет вообще; manual не может «попросить» `ActuatorService` снять маски — у него отдельная точка `tickManual`, отсутствующая в FIELD.

`BurnerState` доступен в manual-сборке (исключается только `burner_fsm.cpp`, не `burner_types.h`).

Кроме того, `kDefinedBits`/`kManualAllowedMask` фиксируются `constexpr`, а неизвестный бит в `demand` — **отказ всей команды**, не «игнорировать»:

```cpp
constexpr ActuatorBits kDefinedBits = (ActuatorBits(1u) << uint8_t(ActuatorId::Count)) - 1u;
constexpr ActuatorBits kManualAllowedMask = kDefinedBits;   // safety-клапанов в enum нет by design
static_assert((kManualAllowedMask & ~kDefinedBits) == 0);
```

`ProcessImage` — вход управляющего тика:

```cpp
struct ProcessImage {
    uint32_t       nowMs{0};
    RunMode        mode{RunMode::BootSafe};
    SensorSnapshot sensors{};
    SafetyLease    lease{};       // в manual не используется
    SafetyDecision safety{};      // в manual не используется
    FaultBits      faults{};
    bool           latched{false};
    bool           heatRequest{false};
    bool           stopRequest{false};   // локальный стоп (кнопка/консоль) → UserOff
    bool           resetRequest{false};
};
```

---

## 6. Планировщик и время

| Задача | Период | Бюджет | Примечание |
|---|---:|---:|---|
| `Control` (mode/FSM/interlocks/actuators) | **20 мс** | 2 мс | главный шаг |
| `FastSensors` | 20 мс | 1 мс | flame/discretes |
| `SlowSensors` | 200–250 мс | 2 мс | DS18B20/MAX6675, без ожидания |
| `I2CHealth` | 100 мс | 1 мс | PCF8574 |
| `SafetyLink` TX/RX | **100 мс** | срез ≤1 мс; кадр 9,4 мс — non-blocking drain | AUTO; в manual не инстанцируется |
| `ManualMonitor` | 500 мс | 1 мс | manual only: периодический лог/статус датчиков |
| `Console` | 50 мс | 1 мс | manual only |
| `BenchAutoConsole` | 50 мс | 1 мс | bench-auto only: `status`, `bench banner` |
| `Housekeeping` | 1000 мс | 2 мс | статистика/NVM/лог |
| `WebConfig` | event-driven | ≤2 мс/тик | [M6] HTTP только чанками; сеть никогда не блокирует `Control` |

- Порядок: sensors → safety (AUTO) → mode → FSM/interlocks → actuator apply → link TX → monitor → housekeeping.
- `SafetyLink` 18 байт @19200 ≈ 9,4 мс → non-blocking TX drain, не в 1 мс.
- Heartbeat формируется в цикле FSM (SR-B8/B11); `SafetyLink` только отправляет.
- `esp_task_wdt` timeout 1,5 с; при `overrun` — не кормить, сначала safe-группа.
- Реакция AUTO: `!permitDangerous` → `kDangerGroup` OFF, `!permitHeater` → `kHeaterGroup` OFF ≤ **период `SafetyLink` (100 мс) + 1 тик**. Меньший бюджет невозможен: разрешение приходит по UART. Внутри этого окна физическую отсечку обеспечивает Nano (hardware cutoff/gate), а не Main Unit — иначе заявление было бы ложным.
- В manual WDT/overrun → `allOff`, выход из manual, затем TWDT reset; гарантии OFF после reset **нет** (§11.2), нагрузки снимаются физически.
- `millis()` wrap-safe; пропущенные тики не догоняются пачкой.
- Overrun-детект — в цикле (`missedTicks`), до срабатывания TWDT; `allOff` выполняет `ActuatorService`, не panic-handler.

---

## 7. Домен

### 7.1. FSM горелки

```cpp
enum class BurnerState : uint8_t {
    Init, Idle, PrePurge, Ignition, Run, PostPurge, Shutdown, Fault, Latched
};
enum class StopReason : uint8_t {
    None, HeatDone, UserOff, TrunMax, PermitLost, LinkLoss, SafetyAlarm,
    GateFault, SensorInvalid, FlameSensorInvalid, FlameLoss, ExhaustInvalid,
    IgnitionTimeout, Timeout, I2cFault, ActuatorMismatch, Internal
};

struct BurnerInputs {
    const ProcessImage& in;
    FaultBits           localFaults;
};
struct BurnerDecision {
    BurnerState  next{BurnerState::Idle};
    ActuatorBits demand{kAllOff};          // безопасный дефолт
    StopReason   reason{StopReason::None};
    FaultBits    raise{};
};
class BurnerFsm {
public:
    void           resetToSafe(uint32_t nowMs);
    BurnerDecision step(const BurnerInputs&);
private:
    BurnerState state_{BurnerState::Init};
    uint32_t    stateSinceMs_{0};      // uint32: таймеры не зависят от 16-бит stateAge
    uint16_t    cycle_{0};
    uint8_t     ignitionAttempts_{kMaxIgnitionAttempts};  // NVS; до успешного чтения — исчерпано
    bool        stableRunReached_{false};
    bool        retryPending_{false};
    bool        prevHeatRequest_{false};  // фронт: сброс при boot/Off/Latched
};

// newHeatEdge = heatRequest && !prevHeatRequest_; prevHeatRequest_ = heatRequest
// в конце step(). Владелец фронта — BurnerFsm (не ProcessImage), чтобы
// единственным источником решения оставался FSM.
```

**Таблица переходов (сверху вниз, первое совпадение; latch-строки приоритетны):**

| Из | Условие | В | Действие |
|---|---|---|---|
| `Init` | self-check OK && !latched | `Idle` | все OFF |
| `Init` | latched / self-check fail | `Latched` | OFF |
| `Idle` | latched / `Latch` | `Latched` | OFF |
| `Idle` | `heatRequest` && (`newHeatEdge` \|\| `retryPending`) && `permitProcess` | `PrePurge` | fan ON, клапаны/розжиг OFF; `ignitionAttempts` не сбрасывается, `stableRunReached_=false` |
| `Idle` | `!heatRequest` | `Idle` | OFF |
| `PrePurge` | `Tprepurge` истёк && **`kPurgePathApproved`** && `permitDangerous` && flame sensor valid && flame Off | `Ignition` | fan ON, air MAIN OPEN, spark ON; `ignitionAttempts_++` |
| `PrePurge` | `Tprepurge` истёк && **!`kPurgePathApproved`** | `Fault` | danger OFF, fan ON; `Internal`/`PurgeNotApproved` (retry запрещён) |
| `PrePurge` | `Tprepurge` истёк && !`permitDangerous` | `Fault` | danger OFF, fan ON; `PermitLost` |
| `PrePurge` | таймаут | `Fault` | danger OFF, fan ON; `Timeout` |
| `PrePurge` | latched | `Latched` | fan OFF |
| `Ignition` | после `TignOilDelay` | `Ignition` | oil MAIN OPEN (spark ON) |
| `Ignition` | flame On ≤ `Tign` **&& `canRun`** | `Run` | spark OFF, клапаны OPEN |
| `Ignition` | `Tign` истёк без пламени | `Fault` | **danger OFF**, fan ON; `IgnitionTimeout` |
| `Ignition` | !`permitDangerous` | `Fault` | **danger OFF**, fan ON; `PermitLost` |
| `Run` | `!heatRequest` | `PostPurge` | danger OFF, fan ON; `HeatDone` |
| `Run`/`Ignition`/`PrePurge` | `stopRequest` | `PostPurge` (`PrePurge` → `Idle`) | danger OFF; `UserOff` |
| `Run` | `TrunMax` достигнут | `PostPurge` | danger OFF, fan ON; `TrunMax` |
| `Run` | стабильная работа `Tstable` | `Run` | `stableRunReached_=true`, `ignitionAttempts_=0` |
| `Run` | flame loss | `Fault` | **danger OFF**, fan ON; `FlameLoss` |
| `Run` | exhaust invalid | `Fault` | **danger OFF**, fan ON; `ExhaustInvalid` |
| `Run` | !`permitDangerous` | `Fault` | **danger OFF**, fan ON; `PermitLost`/`SafetyAlarm`/`GateFault` |
| `Run` | critical local fault | `Fault` | **danger OFF**, fan ON; `I2cFault`/`ActuatorMismatch`/`Internal` |
| `PostPurge` | `Tpostpurge` истёк && !latched | `Idle` | fan OFF, danger OFF |
| `PostPurge` | любой fault/latch | `Fault` | danger OFF, продолжить постпродувку |
| `Fault` | `TpostpurgeFail` истёк && `retryAllowed` && `heatRequest` && `permitProcess` | `Idle` | fan OFF, danger OFF; `retryPending_=true` |
| `Fault` | `TpostpurgeFail` истёк && иначе | `Latched` | все OFF, latch |
| `Latched` | условия §7.4 | `Idle` | latch снят |
| `Ignition`/`Run`/`PostPurge` | latched | `Fault` | **danger OFF**, fan ON |

`PrePurge` при latch обрабатывается строкой выше (`PrePurge | latched | Latched`), дубля нет.

**Правила:**

- `PostPurge` — **только штатный останов** (`HeatDone`, `UserOff`, `TrunMax`). Любой fault → `Fault`.
- `Fault` всегда включает постпродувку (fan ON) на `TpostpurgeFail` (≥ `Tpostpurge`) и затем либо `Idle` (retry allowed), либо `Latched`.
- **Инвариант §5.1 действует всегда**: в `Fault`/`PostPurge`/`Shutdown`/`Latched` `(demand & (kFuelGroup|kSparkGroup)) == 0` независимо от `permitDangerous`.
- `Ignition`: spark ON + air MAIN OPEN; oil MAIN OPEN после `TignOilDelay`; пламя в `Tign`. Переход в `Run` — только через `canRun` (exhaust valid, `kValidAirPressure`, `permitDangerous`), а не по одному факту пламени.
- «Пламя устойчиво `On`» для `canRun` = `FlameStatus::On` от утверждённого датчика (BLK-005) непрерывно не менее `TflameStable` (старт 3 с, `safety-reviewer`); дребезг сбрасывает таймер.
- `Latched` — **все OFF** (`kAllOff`), включая `PumpBoiler`. Исключений нет.
- `Shutdown` — reserved для будущего power-down (M6); в M0–M4 **недостижим**, присутствует только в enum и инварианте.
- **Владелец `demand` при latch (AUTO) — всегда `BurnerState`.** `ModeManager::Latched` — запись/индикация причины, а не второй источник `demand`: пока FSM находится в `Fault`, применяется постпродувка (fan ON, danger OFF), и только переход FSM в `Latched` даёт `kAllOff`. Строка §8.4 `Latched → kAllOff` читается как «если `BurnerState == Latched`».
- В **Manual Bypass** `BurnerFsm` **не инстанцируется**; `BurnerState` наружу не отдаётся (в heartbeat/логах — `UNKNOWN`); команды прямые; heartbeat не отправляется.

**Максимальный `stateAge` (супервизия Nano):**

| Состояние | Max `stateAgeMs` | Примечание |
|---|---:|---|
| `Init` | 5 с | boot tolerance `INIT` — BLK-006 |
| `Idle` | не ограничен | stall по `fsmCycle` |
| `PrePurge` | `Tprepurge + 5 с` | |
| `Ignition` | `Tign + 2 с` | |
| `Run` | `TrunMax + 10 с` | >65,5 с — см. ниже |
| `PostPurge` | `Tpostpurge + 5 с` | |
| `Fault` | `TpostpurgeFail + 5 с` | до `Latched`/retry |
| `Latched` | не ограничен | |

`stateAgeMs` — 16 бит, насыщение 65,5 с, только heartbeat/диагностика. Внутренние таймеры — `uint32 stateSinceMs_`. Для `Run` с `TrunMax > 60 с` супервизия опирается на `fsmCycle`/`runElapsed`; **BLK-006** должен расширить поле или добавить `runElapsed`.

### 7.2. Interlocks (AUTO)

```cpp
struct InterlockResult {
    bool       allow{false};               // fail-closed
    FaultBits  raise{};
    StopReason reason{StopReason::Internal};
};

InterlockResult canPrePurge(const SensorSnapshot&, const SafetyDecision&);
InterlockResult canIgnite (const SensorSnapshot&, const SafetyDecision&);
InterlockResult canRun    (const SensorSnapshot&, const SafetyDecision&);
InterlockResult canHeatOil(const SensorSnapshot&, const SafetyDecision&);
```

| Переход | Локальные условия | Условия безопасности |
|---|---|---|
| `PrePurge` | нет critical fault; fan доступен | `permitProcess` |
| `Ignition` | `Tprepurge` завершён; flame sensor **valid** и `Off`; fan ON | `permitDangerous`; air pressure valid |
| `Run` | flame устойчиво `On`; exhaust valid | `permitDangerous`; `kValidAirPressure` |
| `HeatOil` | `SEN_OIL_TEMP_MAIN` Ok; `BurnerState ∈ {Idle, Run}`; внешняя тепловая защита — железом | **`permitHeater`** (включает `kValidOilTempSafety` **и** `kValidFloatMin`) |

- Fan feedback отсутствует: «fan ON» — команда, не подтверждение; наддув подтверждается `kValidAirPressure` (Nano).
- `NotFitted`/`Stale`/`Invalid` требуемого датчика — **отказ**.
- Критичные Nano-датчики не проверяются локально как источник истины.
- `FlameStatus::On` только от утверждённого датчика (BLK-005).
- В Manual Bypass интерlockи **не применяются**; датчики только мониторятся.
- `canHeatOil` вычисляется каждый тик для `Idle`/`Run` и передаётся в `ActuatorDemand` вместе с `permitHeater`; при невалидном `SEN_OIL_TEMP_MAIN`/`kValidOilTempSafety`/`kValidFloatMin` ТЭН OFF.

### 7.3. SafetyPolicy, SafetySupervisor и lease (AUTO)

```cpp
// domain/safety_types.h — доменные типы, без common_protocol.h
enum class GateState  : uint8_t { Unknown, Closed, OpenSafe, Latched, Fault };
enum class ContactorFb : uint8_t { Unknown, Off, OnConfirmed, Fault };
enum class PowerFb     : uint8_t { Unknown, Present, Absent, Fault };
using AlarmBits       = uint16_t;
using SensorValidBits = uint16_t;

// Доменные биты валидности. SafetyLink обязан доказать static_assert'ом,
// что они совпадают с protocol-битами; domain common_protocol.h НЕ включает.
constexpr SensorValidBits kValidAirPressure   = 1u << 0;
constexpr SensorValidBits kValidOilTempSafety = 1u << 1;
constexpr SensorValidBits kValidFloatMin      = 1u << 2;
constexpr SensorValidBits kValidFloatMax      = 1u << 3;

constexpr SensorValidBits kRequiredSensors =
    kValidAirPressure | kValidOilTempSafety | kValidFloatMin | kValidFloatMax;
// ТЭН: критичен минимум масла в подогреваемой ёмкости ("не на сухую").
constexpr SensorValidBits kHeaterRequiredSensors = kValidOilTempSafety | kValidFloatMin;

struct SafetyLease {
    bool           valid{false};
    bool           everValid{false};
    bool           permit{false};
    bool           latched{false};
    GateState      gateState{GateState::Unknown};
    AlarmBits      alarmBits{0};
    SensorValidBits sensorValid{0};
    ContactorFb    contactorFb{ContactorFb::Unknown};
    PowerFb        kc868PowerFb{PowerFb::Unknown};
    uint32_t       rxStampMs{0};   // локальное время приёма
};

enum class SafetyVerdict : uint8_t { Allow, Deny, Latch };

struct SafetyDecision {
    SafetyVerdict verdict{SafetyVerdict::Deny};
    bool          permitProcess{false};    // fan/prepurge
    bool          permitDangerous{false};  // fuel/spark/air
    bool          permitHeater{false};     // ТЭН: контактор+oil-temp, БЕЗ gate OPEN_SAFE
    bool          latchRequest{false};
    StopReason    reason{StopReason::PermitLost};
};

struct SafetyConfig {              // compile-time, НЕ из NVS
    bool requireContactorFb{false};
    bool requireKc868PowerFb{false};
};
// В FIELD+OILCTRL_ENABLE_AUTO оба обязаны быть true — #error в include/build_profile.h (§10.1).

// История, которой policy не может владеть (чистая функция получает её явно).
struct SafetyHistory {
    bool everValid{false};
    bool everPermitted{false};
    bool everContactorConfirmed{false};
    bool everPowerConfirmed{false};
};

// Чистая функция: lease+history+faults+cfg -> SafetyDecision.
// Единственный stateful-арбитр — SafetySupervisor ниже.
SafetyDecision safetyPolicyEvaluate(const SafetyLease&, const SafetyHistory&,
                                    const FaultBits&, uint32_t nowMs, const SafetyConfig&);

class SafetySupervisor {
public:
    void reset(uint32_t nowMs);
    SafetyDecision evaluate(const SafetyLease&, const FaultBits& localFaults,
                            uint32_t nowMs, const SafetyConfig& cfg);
private:
    SafetyHistory history_{};            // единственный владелец ever*; передаётся в policy
};
```

**Условия (AUTO):**

```
baseNoSensors = valid && permit && !latched && alarmBits == 0
                && (nowMs - rxStampMs) <= 600
sensorsOk     = (sensorValid & kRequiredSensors) == kRequiredSensors
gateSafe      = gateState == GateState::Closed || gateState == GateState::OpenSafe
contactorOk   = !cfg.requireContactorFb || contactorFb == ContactorFb::OnConfirmed
powerOk       = !cfg.requireKc868PowerFb || kc868PowerFb == PowerFb::Present

danger        = baseNoSensors && sensorsOk && gateState == GateState::OpenSafe
                && powerOk && contactorOk

permitProcess   = baseNoSensors
permitDangerous = danger
permitHeater    = baseNoSensors && gateSafe && contactorOk && powerOk
                  && (sensorValid & kHeaterRequiredSensors) == kHeaterRequiredSensors
```

`permitHeater` **не требует** `gateState == OPEN_SAFE`: ТЭН не подаёт топливо и не участвует в розжиге. Но он требует **минимум масла** (`kValidFloatMin`) и валидную температуру — иначе 220 В нагревали бы пустую ёмкость. `powerOk` учитывается так же, как для опасной группы. `gateState == FAULT/LATCHED/UNKNOWN` уже даёт `Latch` строками таблицы.
`kRequiredSensors` — осознанно консервативно; разделение по функциям — BLK-006/политика.

**Таблица решений:**

| Ситуация | verdict | process | dangerous | latch |
|---|---|---|---|---|
| `!valid && !everValid` | Deny | ✗ | ✗ | ✗ |
| `!valid && everValid` | Latch | ✗ | ✗ | ✓ |
| `valid && latched` | Latch | ✗ | ✗ | ✓ |
| `valid && alarmBits != 0` | Latch | ✗ | ✗ | ✓ |
| `valid && !permit && everPermitted` | Latch | ✗ | ✗ | ✓ |
| `valid && !permit && !everPermitted` | Deny | ✗ | ✗ | ✗ |
| `gateState ∈ {FAULT, UNKNOWN}` | Latch | ✗ | ✗ | ✓ |
| `gateState == LATCHED` | Latch | ✗ | ✗ | ✓ (Nano уже латчился) |
| `gateState == CLOSED` | Allow | ✓ | ✗ | ✗ (permitHeater — по отдельному условию ниже) |
| `cfg.requireKc868PowerFb && kc868PowerFb == FAULT` | Latch | ✗ | ✗ | ✓ |
| `cfg.requireKc868PowerFb && everPowerConfirmed && kc868PowerFb != PRESENT` | Latch | ✗ | ✗ | ✓ |
| `cfg.requireKc868PowerFb && !everPowerConfirmed && kc868PowerFb != PRESENT` | Allow | ✓ | ✗ | ✗ |
| `cfg.requireContactorFb && contactorFb == FAULT` | Latch | ✗ | ✗ | ✓ |
| `cfg.requireContactorFb && everContactorConfirmed && contactorFb != ON_CONFIRMED` | Latch | ✗ | ✗ | ✓ |
| `cfg.requireContactorFb && !everContactorConfirmed && contactorFb != ON_CONFIRMED` | Allow | ✓ | ✗ | ✗ |
| `baseNoSensors && gate CLOSED && contactor/power OK && heaterSensors OK` | Allow | ✓ | ✗ | ✗ (permitHeater ✓) |
| `baseNoSensors && !sensorsOk` | Allow | ✓ | ✗ | ✗ (permitHeater — только если `kHeaterRequiredSensors` валидны) |
| `baseNoSensors && sensorsOk && gate OPEN_SAFE && feedbacks OK` | Allow | ✓ | ✓ | ✗ |
| `localFaults` CRITICAL/HIGH | Latch | ✗ | ✗ | ✓ |

- `verdict` — только про **process**; опасные актуаторы разрешает только `permitDangerous`, ТЭН — только `permitHeater`.
- **Владельцы состояния:** `SafetyLink` — `valid`/`rxStampMs` (сырые кадры); `SafetySupervisor` — `SafetyHistory` (`everValid`/`everPermitted`/`everContactorConfirmed`/`everPowerConfirmed`) и передаёт её в policy. `everPermitted` выставляется при первом `valid && permit` и не сбрасывается без `reset`; иначе потеря `permit` после первого разрешения не давала бы `Latch`. `SafetyLink` **не** владеет `ever*` — иначе два владельца истории.
- `Latch` немедленно ставит локальный latch через `ModeManager`.
- `SafetyLink` — владелец 600 мс, CRC/seq/protoVer, replay, N=3 кадров.
- Потеря `ON_CONFIRMED`/`PRESENT` после подтверждения — `Latch`.
- **Feedback handshake (BLK-002/BLK-008):** Nano замыкает контактор независимо от `fsmState`; `permit → ON → ON_CONFIRMED → permitDangerous`.
- **Семантика `safetyPermit` (BLK-006):** `permit` — технологическое разрешение; опасные дополнительно требуют `gate OPEN_SAFE`; Nano выставляет `permit` независимо от `fsmState`.
- `requireContactorFb`/`requireKc868PowerFb` — compile-time gate. `false` допустим **только** до подключения обратной связи. В сборке `OILCTRL_ENABLE_AUTO=1` (M4) оба **обязаны** быть `true`; иначе `include/build_profile.h` даёт `#error` (`OILCTRL_REQUIRE_CONTACTOR_FB`/`OILCTRL_REQUIRE_KC868_POWER_FB`), и негативная сборка включена в CI (§10.1).
- **`SafetyPolicy` — чистая функция** `safetyPolicyEvaluate(lease, faults, now, cfg) → SafetyDecision` (без состояния, host-тестируема). `SafetySupervisor` — stateful-обёртка (`everValid`/`everPermitted`/`everContactorConfirmed`/`everPowerConfirmed`) и **единственный** арбитр (ADR-15). В manual-сборку не входит ни один из них.
- `SafetyPolicy` не имеет своего `demand`, не хранит состояния и не пишет актуаторы: только `SafetyDecision`. Stateful-строки таблицы (`ever*`) вычисляются по `SafetyHistory`, которую ведёт supervisor.
- `SafetyLink` доказывает соответствие доменных `kValid*`/`GateState`/`ContactorFb`/`PowerFb` протоколу через `static_assert` (host-TU); несовпадение — ошибка сборки.
- **В Manual Bypass этот блок не используется**: supervisor не инстанцируется, lease не читается.

### 7.4. Снятие latch (AUTO)

Единый latch в `ModeManager`; `BurnerState::Latched` — отражение. Разделяются:

- `activeCondition` — причина присутствует **сейчас** (проверяется каждым тиком);
- `latchedRecord` — запись о срабатывании (персистится, NVS с M4).

Условия `reset`:

1. Явный `resetRequest` — **фронт** нажатия (one-shot, debounce 50 мс), не уровень: удержание кнопки не снимает latch повторно. Локальная кнопка/консоль; не RS485/Wi-Fi. Залипание входа `reset`/`stop` дольше 5 с ⇒ `Fault(Internal)`, а не бесконечный reset.
2. `verdict != Latch`, свежий валидный lease (≥3 кадра), `permitProcess`.
3. `gateState ∈ {OpenSafe, Closed}`, `alarmBits == 0`, обязательные `sensorValid`.
4. Постпродувка завершена, `stateAge` в `Latched` ≥ `TlatchMin`.
5. Нет **активных** CRITICAL/HIGH `activeCondition`.

Порядок снятия (обязателен): `resetRequest` → проверка 2–5 → снятие **supervisor latch** (Nano-подтверждение через BLK-006) → очистка `latchedRecord` → `Idle`. Иначе возможен deadlock: липкий `F_FlameLoss` навсегда блокирует условие 5.

- `reset` очищает `ignitionAttempts` (иначе после 3-й неудачной попытки `Latched → Idle → Ignition` снова forbidden и горелка невосстановима без стирания NVS).
- Часовой rate-limit (`kMaxIgnitionsPerHour`) **не** очищается reset; окно персистится, сброс — только сервисной процедурой.
- Сброс не снимает физическую причину; восстановление связи само latch не снимает; автосброса нет.

### 7.5. Политика повторного розжига

Реализуется **allow-list**, не deny-list: повторный розжиг разрешён **только** для явно перечисленных причин. Любая другая причина — включая `None`, нормальный останов и любую будущую, добавленную в `StopReason`, — **запрещена**. Это fail-safe-инверсия: забытая причина не может разрешить розжиг.

```cpp
enum class RetryClass : uint8_t { Forbidden, Allowed };

struct RetryContext {
    // FAIL-CLOSED дефолт: пока NVS не прочитан успешно, лимит считается исчерпанным.
    uint8_t  ignitionAttempts{kMaxIgnitionAttempts};
    uint32_t lastFaultMs{0};
};

// Реализация — switch без ветки "разрешено по умолчанию":
RetryClass classifyRetry(StopReason reason, const RetryContext& ctx) {
    switch (reason) {
        case StopReason::IgnitionTimeout:
            return ctx.ignitionAttempts < kMaxIgnitionAttempts
                   ? RetryClass::Allowed : RetryClass::Forbidden;
        default:                       // None, FlameLoss, нормальный останов, будущие
            return RetryClass::Forbidden;
    }
}
```

**Запрещено — нормальный останов (без latch, новый пуск только по новому фронту `heatRequest`):**

| Причина | Почему |
|---|---|
| `HeatDone`, `UserOff`, `TrunMax` | штатное завершение: `Fault`/`Latch` не выставляются, `ignitionAttempts` сбрасывается |

**Запрещено — hard latch (только явный reset §7.4):**

| Причина | Почему |
|---|---|
| `None` | нет причины — нет права на розжиг |
| `PermitLost`, `LinkLoss`, `SafetyAlarm`, `GateFault` | потеря разрешения/связи/авария Nano |
| `I2cFault`, `ActuatorMismatch` | недостоверное управление |
| `SensorInvalid` (критичные Nano-датчики) | недостоверная защита |
| `FlameSensorInvalid`, `FlameLoss` | датчик пламени недостоверен / потеря пламени. `modules.md`, `checklist.md`, `periph_distribution.md`, `blockers.md` (BLK-005) требуют **latch/no auto-restart** |
| `ExhaustInvalid` | граница BLK-003: latch |
| `Timeout` | застревание состояния |
| `Internal` | внутренняя ошибка |
| `IgnitionTimeout` при `ignitionAttempts >= kMaxIgnitionAttempts` | исчерпан лимит попыток |
| Любая причина, не входящая в allow-list | fail-safe default |

**Разрешено (bounded retry):**

| Причина | Условия (все обязательны) |
|---|---|
| `IgnitionTimeout` | `ignitionAttempts < kMaxIgnitionAttempts`; `heatRequest`; свежий `permitProcess`; `Tprepurge`/`TpostpurgeFail` проверены на котле; в протоколе есть `attemptNo`/`retryCount`, Nano разрешает `FAULT → IDLE → PREPURGE` в пределах лимита |

`FlameLoss` **не имеет retry**. Возврат возможен только отдельным осознанным решением с записью в `accepted_risks.md`; «побочным эффектом» архитектуры не вводится.

**Механика:**

- Каждый retry — **полная** `PrePurge`, не прыжок в `Ignition`.
- После `Fault` — **отдельная** постпродувка `TpostpurgeFail ≥ Tpostpurge`, чтобы вынести остатки топлива после неудачного розжига. Оба значения проверяются на котле **до** включения retry; до этого `kMaxIgnitionAttempts = 1` (retry выключен).
- `ignitionAttempts_++` и **write-ahead persist в NVS — до** входа в `Ignition`; ошибка записи NVS ⇒ розжиг запрещён (`Latched`). Сброс — при штатном останове после `Run`/`Tstable` и при явном `reset` (§7.4).
- `stableRunReached_ = false` — при входе в `PrePurge` (иначе флаг «протекает» на следующий цикл).
- `retryPending_` сбрасывается при входе в `PrePurge`.
- `heatRequest` обязателен на каждом цикле; `retryPending_` не переживает снятие запроса тепла.
- `Fault` после `TpostpurgeFail`: `classifyRetry == Allowed && heatRequest && permitProcess` → `Idle` + `retryPending_`; иначе `Latched`.
- Каждый retry требует **нового** feedback-handshake контактора (ADR-17): после fault Nano мог разомкнуть контактор, `ON_CONFIRMED` должен быть получен заново.
- **Персистентность:** `ignitionAttempts` и окно частоты — NVS. **Corrupt/missing/ошибка чтения ⇒ fail-closed:** `ignitionAttempts = kMaxIgnitionAttempts` (дефолт поля, а не `0`), окно = «исчерпано», retry запрещён до сервисного сброса. Reboot не обнуляет лимит.
- **Rate limit:** не более `kMaxIgnitionsPerHour` (старт 10) за скользящий час; превышение → `Latched`. Окно хранится как `monotonic uptime + bootCounter`; при неизвестном/сброшенном времени — **fail-closed** (retry запрещён до валидного окна).

**Severity и жизненный цикл fault (единая таблица, синхронна §8.2):**

| Причина | FaultBits / severity | Очистка | Latch supervisor |
|---|---|---|---|
| `IgnitionTimeout` | `F_IGNITION_TIMEOUT` / MEDIUM | **на переходе** `Fault→Idle/Latched` | нет (пока retry разрешён) |
| `FlameLoss` | `F_FLAME_LOSS` / HIGH | sticky до явного `reset` | да |
| `FlameSensorInvalid` | `F_FLAME_SENSOR_INVALID` / HIGH | sticky до явного `reset` | да |
| `PermitLost`/`LinkLoss`/`SafetyAlarm`/`GateFault` | соответствующий / HIGH | sticky до явного `reset` | да |
| `I2cFault`/`ActuatorMismatch`/`Internal` | соответствующий / HIGH | sticky до явного `reset` | да |
| `ExhaustInvalid` | `F_EXHAUST_INVALID` / HIGH | sticky до явного `reset` | да |
| `SensorInvalid` | `F_SENSOR_INVALID` / HIGH | sticky до явного `reset` | да |
| `Timeout` | `F_TIMEOUT` / HIGH | sticky до явного `reset` | да |

`F_IGNITION_TIMEOUT` не показывается как «норма»: остаётся активным до конца цикла и входит в `errSeverity` heartbeat. LOW/MEDIUM, не перечисленные здесь, автоочищаются по таймеру; CRITICAL/HIGH — только через §7.4.

**Владелец latch при `IgnitionTimeout`:** `BurnerState::Latched` ставит `ModeManager` (локальный latch), **не** `SafetySupervisor`; снимается по §7.4. Supervisor latch по этой причине не выставляется, иначе retry-цикл был бы невозможен в принципе.

**Лимиты** (`constexpr`, `config/limits.h`, не runtime-tunable):

| Константа | Старт | Условие изменения |
|---|---:|---|
| `kMaxIgnitionAttempts` | 1, далее 3 | после проверки `Tprepurge`/`TpostpurgeFail` на котле |
| `kMaxIgnitionsPerHour` | 10 | `safety-reviewer` |
| `Tstable` | 60 с | `safety-reviewer` |
| `TflameStable` | 3 с | `safety-reviewer` (устойчивость пламени для `Run`) |
| `TpostpurgeFail` | `Tpostpurge + 30 с` (TBD) | проверка на котле |

`static_assert(TrunMax > Tstable + margin)` обязателен: иначе «стабильная работа» недостижима, счётчики не сбрасываются и нормальные пуски приводят к ложному latch.

---

## 8. Сервисы

### 8.1. ActuatorService — единственный писатель

```cpp
// services/actuator_service.h — общий заголовок, без #if и без manual-API
// ActuatorDemand определён в §5.1 (bits + state + permitDangerous/permitHeater/allowHeatOil)

class ActuatorService {
public:
    void beginSafe();
    void tickAuto(uint32_t nowMs, const ActuatorDemand&);  // FIELD: маски и инвариант §5.1
    // Объявлена всегда, ОПРЕДЕЛЕНА только в manual/actuator_service_manual.cpp.
    // В FIELD символа нет: любой вызов = ошибка линковки, а не runtime-обход.
    void tickManual(uint32_t nowMs, ActuatorBits bits);
    ActuatorBits applied() const;
    Readback readback(ActuatorId) const;
    bool readbackMismatch() const;
    bool outputsUnknown() const;                        // I2C fault: фактическое состояние неизвестно
private:
    ActuatorBits applied_{kAllOff};
    bool         outputsUnknown_{false};
};

// manual/manual_actuator_port.h — ТОЛЬКО manual-сборка
class IManualActuatorPort {
public:
    virtual ~IManualActuatorPort() = default;
    virtual bool setManual(ActuatorId id, bool on, uint32_t nowMs) = 0;   // по одному актуатору
    virtual void allOffManual(uint32_t nowMs) = 0;
};
```

- **`IManualActuatorPort` реализует `ManualControlService`** — единственный владелец вектора. `ActuatorService` **не реализует** этот порт и не наследует ничего manual-специфичного (иначе `services -> manual`, запрещено §3).
- **Изоляция manual-пути — компоновочная, а не runtime.** `tickManual` только **объявлен** в общем заголовке; его определение лежит в `manual/actuator_service_manual.cpp`, который не входит в FIELD-фильтр. В FIELD символа `tickManual` нет: случайный вызов — ошибка линковки. Никакого `autoMode=false`/`bool manual` в FIELD-API не существует, поэтому «выключить маски» из FIELD-кода невозможно в принципе.
- Manual-цикл композиции: `ManualControlService::tick(now)` (таймеры/deadman/max-on) -> `ActuatorService::tickManual(now, manual.bits())`. Вектор один, второй копии нет, поэтому `set fan 1` + `set oil 1` не гасят вентилятор. `tickManual` — та же **единственная** точка записи в железо.
- `setManual` **валидирует и обновляет вектор владельца**, но не пишет в железо; применение — только `tickAuto`/`tickManual` (общий приватный `apply()`). `false` при: `!manualEnabled_`, неизвестном/недопустимом `ActuatorId`, `outputsUnknown_`, незавершённом `min-off` этого актуатора. `false` -> бит не меняется, причина логируется.
- Явная ON-команда (`set x 1`) **стартует новое окно** max-on/deadman для этого актуатора; истёкшее окно не блокирует повторное включение (иначе оператор не смог бы вернуть актуатор после автo-OFF). Повторный `set x 1` при включённом `x` окно не сбрасывает — только `keep`.
- **Две точки входа, один писатель.** `tickAuto(now, demand)` — FIELD: применяет инвариант §5.1 по `demand.state` и обе маски. `tickManual(now, bits)` — manual: пишет переданный вектор, но **символа нет в FIELD** (определение в `manual/`), поэтому runtime-переключателя не существует. Обе точки используют один приватный `apply()`, второй копии состояния и второго писателя нет.
- Boot: direct GPIO OFF до init; PCF8574 — §11.1, boot-safe не заявляется.
- AUTO: `!permitDangerous` → `kDangerGroup` OFF, `!permitHeater` → `kHeaterGroup` OFF в том же тике; `FanBlower` — по FSM; `PumpBoiler` — BLK-007. Инвариант §5.1 проверяется перед применением.
- Read-back: `Readback {NotAvailable, Off, On, Unknown}`. PCF8574 read-back = output latch, **не** контакт; сварка недетектируема. `NotAvailable` ≠ «OFF подтверждён».
- I2C fault/read-back mismatch: в AUTO — `outputsUnknown_=true`, `kSafetyOffGroup` OFF, `actBits` в heartbeat консервативный (`outputsUnknown`/`actValid=false` после BLK-006), bounded retry, latch; в Manual — `outputsUnknown_=true`, `allOffManual`, **принудительный выход `Manual → Off`**, новые команды отклоняются до успешной реинициализации, в лог — «ОТКЛЮЧИТЕ ПИТАНИЕ НАГРУЗОК».
- `NotFitted` датчика и `outputsUnknown_` — **разные** состояния: первое относится к quality входа, второе — к достоверности выходов; `NotFitted` не снимает `outputsUnknown_`.
- `outputsUnknown_` снимается **только** после успешной записи `kAllOff` **и** read-back всех выходов `Off` (плюс успешная реинициализация I2C). До этого `actValid=false`, опасная группа OFF, вход в `Manual` запрещён.
- В Manual `HeaterOil` — обычный актуатор `kHeaterGroup`: разрешён без `canHeatOil` (оператор видит температуру), ограничен общими max-on/min-off/deadman. Внешний термостат и термопредохранитель при этом обязательны — это физика, а не код (AR-06).

**Лимиты (manual — обязательные, не «опция 0 = off»):**

| Выход | Manual max-on | Min-off | Manual deadman |
|---|---:|---:|---:|
| Fan | 1800 с | 0 | 0 (процессная группа) |
| Oil/Air valve | `TrunMax + 10 с` | 1 с | 10 с (опасная группа) |
| Ignition | `Tign + 2 с` | 5 с | 10 с (опасная группа) |
| Heater | `TmaxHeat + 30 с` | 30 с | 10 с (опасная группа) |
| PumpOilTransfer | `TmaxTransfer + 30 с` | 1 с | 10 с (опасная группа) |
| PumpBoiler | 1800 с | 1 с | 0 (процессная группа) |

- `deadman` — **per-actuator** автo-OFF, если оператор не продлил **этот** актуатор за `OILCTRL_MANUAL_DEADMAN_MS` (10 000 мс для опасной группы; 0 — процессная). Продлевают только `keep` и команды, меняющие состояние этого актуатора; `status`, `sensors`, `all off`, команды других актуаторов — **не продлевают**.
- max-on/min-off/deadman проверяются **периодически** в `ManualControlService::tick` (20–100 мс), а не только в момент команды: одна команда без последующих действий гарантированно снимается по таймауту.
- Отключение ограничений — только двумя **независимыми** флагами: `OILCTRL_MANUAL_ALLOW_NO_MAXON` и `OILCTRL_MANUAL_ALLOW_NO_DEADMAN`. Deadman остаётся обязательным, даже если max-on отключён. По умолчанию оба запрещены: это единственное, что не даёт забытой команде держать топливо открытым.
- `FIELD_CEILING(*)` — **compile-time верхняя граница значений max-on**, а не второй runtime-ограничитель. В manual существует **ровно один** runtime-limiter — `ManualControlService::tick`; независимой защиты в manual нет (§17). Это зафиксировано явно, чтобы документ не заявлял defense-in-depth, которого нет.

`config/limits.h` `static_assert`: `TignOilDelay + margin < Tign`, `Tprepurge > 0`, `Tpostpurge > 0`, `TpostpurgeFail >= Tpostpurge`, `TrunMax > Tstable + margin`, `FIELD_CEILING(ignition) ≥ Tign + 2 с`, `FIELD_CEILING(valves) ≥ TrunMax + 10 с`, `FIELD_CEILING(heater) ≥ TmaxHeat + 30 с`, `MANUAL_MAX_ON(danger) ≤ FIELD_CEILING(danger)`, `MANUAL_DEADMAN_MS > 0 && ≤ 10 000` для опасной группы (если не `ALLOW_NO_DEADMAN`), `TlatchMin > 0`.
- Вход в `Manual` требует: I2C healthy, `!outputsUnknown_`, read-back **всех** выходов == `Off`. Иначе `manual on` отклоняется: «все выходы OFF до команд» иначе неверно.

### 8.2. SensorService

- Асинхронные DS18B20, MAX6675; `raw → validity → filter → SensorReading`.
- `Stale/Invalid/NotFitted`; в manual отсутствие датчика **не блокирует** команды, но отображается.
- Критичные Nano-датчики в AUTO — только через lease.
- `FlameStatus::On` только от утверждённого датчика (BLK-005).
- Fault: жизненный цикл — по единой таблице §7.5; LOW/MEDIUM — по таймеру/переходу, CRITICAL/HIGH — sticky до явного `reset`.

### 8.3. SafetyLink и heartbeat (AUTO)

- `ILinkTransport` ↔ `SafetyLink`; M3a TX, M3b RX после BLK-006.
- До M3b: `NullSafetyLink` (`valid=false`), AUTO недостижим.
- Владелец `valid/everValid/rxStampMs`, 600 мс, CRC/seq/protoVer, replay, N=3.
- **Маппинг protocol→domain — только здесь:** `gateState`/`alarmBits`/`contactorFb` переводятся в доменные типы в `SafetyLink`, с `static_assert` на значения (fail-safe полярность `gateState`). Домен не включает `common_protocol.h`.
- `actBits = applied`, но при `outputsUnknown_` — консервативно: `actValid=false`/`outputsUnknown` (поле BLK-006); «applied» не выдаётся за подтверждённое OFF.
- **`uart.md` и `common_protocol.h` сейчас говорят «requested»** — приводим к `applied` + `actValid`/`outputsUnknown`. Пока это не сделано, `actBits` не является доказательством состояния; Nano не должен опираться на него как на подтверждение.

**Heartbeat:**

| Поле | Источник | Правило |
|---|---|---|
| `protoVer`, `seq` | `SafetyLink` | mod 256 |
| `fsmState` | FSM | `UNKNOWN/INIT` до шага |
| `stateAgeMs` | FSM | насыщение 65535; диагностика |
| `fsmCycle` | FSM | конец шага, mod 65536 |
| `i2cPollCycle` | I2C | успешная транзакция+read-back |
| `i2cStatus` | I2C | OK/FAULT/RECOVERED |
| `flameStatus` | SensorService | до BLK-005 — INVALID; ON только утверждённый |
| `actBits` | ActuatorService | applied (не «requested») |
| `actValid` | ActuatorService | `false` при `outputsUnknown_`/`readbackMismatch`; BLK-006 |
| `outputsUnknown` | ActuatorService | I2C fault: фактическое состояние неизвестно; BLK-006 |
| `attemptNo`/`retryCount` | FSM | **только после расширения BLK-006**; без поля retry не реализуется |
| `errSeverity` | Diagnostics | максимум активных |
| `crc8` | SafetyLink | BLK-006 |

### 8.4. ModeManager

```cpp
enum class RunMode : uint8_t {
    BootSafe = 0, Off, Manual, Auto, Fault, Latched   // Manual присутствует ВСЕГДА
};
```

- `Manual` в enum есть во всех сборках (ADR-11). `ModeManager` **не знает про `IManualRequest`**: порт опрашивает `app/`, который передаёт `bool manualRequested` в `ModeManager::request(...)`. Тем самым `domain -> hal` не возникает (§3).
- В FIELD `app_field.cpp` использует `NullManualRequest` (класс живёт в `app/`, всегда `false`), поэтому у перехода в `Manual` нет вызывающего, а manual-API отсутствует в ELF. Проверяется host-тестом `ModeManager` с фейком `IManualRequest(false)` (не composition root) и `nm`.
- `BurnerState` — safety-состояние; `RunMode` — выбор источника `demand`. `ModeManager` **не пишет** `BurnerState`; в manual `BurnerFsm` не инстанцируется, наружу — `UNKNOWN`.
- Инвариант соответствия: `RunMode::Fault` **согласован** с `BurnerState::Fault` (или `PostPurge` при идущей постпродувке); `RunMode::Latched` — **запись о срабатывании**, при этом `BurnerState` может ещё находиться в `Fault` до конца `TpostpurgeFail`; `RunMode::Auto ⇒ BurnerState ∈ {PrePurge…PostPurge}`. `RunMode::Fault`/`Latched` — **отражение**, а не второй источник; host-тест «`RunMode::Latched` не снимает fan/постпродувку».
- `Off → Manual` — только manual-сборка, по `manual on`; **без** supervisor/lease/interlocks.
- `Manual → Off` — `manual off`/`all off`/I2C fault/deadman.
- `Off → Auto` — только FIELD: `permitProcess` + нет latch. `heatRequest` **не требуется**: иначе предпусковой подогрев масла в `Idle` недостижим (`Off` = `kAllOff`, и FSM не работает). Вход в `PrePurge` по-прежнему требует `heatRequest` + `newHeatEdge`; в `Auto/Idle` опасная группа запрещена инвариантом §5.1, поэтому переход не даёт опасных прав.
- `Fault/Latched → Off` — только AUTO reset.
- Ровно один источник: `Auto` / `Manual` / `kAllOff`.
- Manual не персистится; reboot → `Off`. **Но** состояние выходов PCF8574 при reset MCU не сбрасывается (§11.1) — «reboot → Off» относится только к `RunMode`.

| RunMode | BurnerState | Источник demand | FIELD |
|---|---|---|---|
| `BootSafe` | `Init` | `kAllOff` | да |
| `Off` | `Idle` | `kAllOff` | да |
| `Manual` | `UNKNOWN` (FSM не создан) | manual direct | **переход отсутствует** |
| `Auto` | `Idle` (ожидание/подогрев) … `PostPurge` | FSM | да (M4+) |
| `Fault` | `Fault` | постпродувка | да |
| `Latched` | `Fault` (постпродувка идёт) | постпродувка | да |
| `Latched` | `Latched` | `kAllOff` | да |

`RunMode::Latched` — **не команда «всё выключить»**, а запись о срабатывании. Пока `BurnerState == Fault`, постпродувка продолжается; `kAllOff` наступает только при переходе FSM в `Latched`. Источник `demand` — всегда `BurnerState`. Таблица читается сверху вниз, первая совпавшая строка.

### 8.5. Diagnostics

- `FaultCode/Severity`; липкие fault; причина останова с кодом.
- Порядок critical: safe → bounded NVS → лог.
- `esp_reset_reason()`, boot counter, build hash/profile.
- Счётчики: `fsmCycle`, `i2cPollCycle`, `missedTicks`, `linkTimeouts`, `ignitionAttempts`, `retryCount`.
- Rate limit логов; ring buffer; без `String`/heap в критичном пути.

### 8.6. ManualConsole и ManualMonitor (только manual-сборка)

```
manual> manual on
manual> manual off
manual> set fan 1
manual> set oil 1
manual> set air 1
manual> set ignition 1
manual> set heater 1
manual> set pump_transfer 1
manual> set pump_boiler 1
manual> set all 0
manual> keep
manual> sensors
manual> status
```

- `manual on` — вход в режим; все выходы остаются OFF до команд.
- `set ... 0`, `set all 0`, `manual off` — немедленно, без подтверждения.
- `set` — прямое управление, **без** arm/keep-цикла/permit/interlocks. `keep` только продлевает deadman.
- `sensors`/`status` — значения и quality всех доступных датчиков; `NotFitted/Invalid` отображаются, не блокируют команды.
- `status` обязан показывать: активный deadman, остаток max-on, `outputsUnknown` (если I2C fault), и строку `MANUAL BYPASS — SAFETY SUPERVISOR DISABLED`.
- `ManualMonitor` каждые 500 мс логирует snapshot датчиков, I2C health, applied outputs; **не является interlock**, не пишет в `ActuatorService`, его отказ не меняет поведение выходов.
- Нет `arm`, нет hardware key. `manual on` не персистится; reboot → `Off` (но состояние реле — см. §11.1).
- Console не использует `String`/heap; неизвестная команда → отказ + лог.
- Manual не управляет `ACT_*_VALVE_SAFETY`; `ACT_*` с неизвестным битом → отказ всей команды.

---

## 9. HAL и платформа

### 9.1. Порты

```cpp
class IClock { public: virtual ~IClock() = default; virtual uint32_t millis() = 0; };

enum class Readback : uint8_t { NotAvailable, Off, On, Unknown };

class IDigitalOut {
public:
    virtual ~IDigitalOut() = default;
    virtual void write(bool on) = 0;
    virtual Readback readback() const = 0;
};

class ITempBus {
public:
    virtual ~ITempBus() = default;
    virtual void requestConversion() = 0;
    virtual bool read(float& outC) = 0;
};

class ILinkTransport {
public:
    virtual ~ILinkTransport() = default;
    virtual size_t read(uint8_t* buf, size_t cap) = 0;
    // Возвращает число принятых байт; частичная запись допустима и
    // означает "продолжить в следующем тике" (non-blocking drain, §6).
    virtual size_t write(const uint8_t* buf, size_t len) = 0;
};

class IExhaustTemp {                 // MAX6675 (SPI), отдельно от DS18B20-шины
public:
    virtual ~IExhaustTemp() = default;
    virtual void requestConversion() = 0;
    virtual bool read(float& outC) = 0;
};

class INvs {
public:
    virtual ~INvs() = default;
    virtual bool read(const char* key, void* out, size_t cap) = 0;
    virtual bool write(const char* key, const void* in, size_t len) = 0;
};

class IHeatDemand  { public: virtual ~IHeatDemand() = default; virtual bool active() = 0; };
class IStopInput   { public: virtual ~IStopInput() = default; virtual bool pressed() = 0; };
class IResetInput  { public: virtual ~IResetInput() = default; virtual bool pressed() = 0; };

// AUTO-only
class IFlameInput { public: virtual ~IFlameInput() = default; virtual FlameStatus read() = 0; };

// manual-переход: в FIELD — NullManualRequest (всегда false), живёт в app/
class IManualRequest { public: virtual ~IManualRequest() = default; virtual bool requested() = 0; };
```

- Барьерных портов больше нет: `IBenchIdentity`, `IManualRuntimeBudget`, `IExternalSupervisorProbe` удалены вместе с барьерами (AR-02…AR-04). В manual-composition ничего не инжектируется.
- `ISafetySource` (`services/safety_source.h`) — не HAL-порт, а сервисный интерфейс: `poll(now)` + `const SafetyLease& lease()`. FIELD-реализация — `SafetyLink`, bench-auto — `NanoStubLink` (`bench/`). Выбор делает composition root; домен видит только `SafetyLease`.
- `IManualRequest` опрашивается в `app/`, в домен передаётся `bool` (§8.4).
- `IExhaustTemp` отделён от `ITempBus`: MAX6675 — SPI, DS18B20 — 1-Wire; общий интерфейс заставил бы HAL знать о протоколе.
- `write()` допускает частичную запись; `SafetyLink` обязан продолжить с `offset`, а не повторять кадр (иначе двойная отправка `seq`).

`Readback::NotAvailable` — «подтверждения нет», не «OFF подтверждён».

### 9.2. Платформа

- `Clock`: `millis()` + wrap-safe; host — управляемые часы.
- `Watchdog`: TWDT; правила §6.
- `Log`: уровни/теги/rate limit; ESP32 `Serial`, host `printf`.
- `NVS`: `oilctrl`, версия, CRC; **corrupt/missing → fail-closed** для safety-счётчиков (retry запрещён); настройки — дефолты (§9.3); прочие значения — safe defaults. Запись вне критичного пути; для счётчиков розжига — write-ahead.

### 9.3. Единый центр настроек (SettingsService)

**Все числа — в `config/`.** Других «настроечных» мест в коде нет.

| Что | Где живёт | Как меняется | Примеры |
|---|---|---|---|
| **Заводские дефолты и диапазоны** | `config/limits.h`, `constexpr` | пересборкой (обычно не нужно) | `Tprepurge`, `Tign`, `TignOilDelay`, `Tpostpurge`, `TpostpurgeFail`, `TrunMax`, `Tstable`, `TflameStable`, `kMaxIgnitionAttempts`, `kMaxIgnitionsPerHour`, `TlatchMin`, `TmaxHeat`, `TmaxTransfer`, manual deadman/max-on |
| **Текущие значения** | `config/settings.h` + NVS | веб с паролем (§9.4), консоль | всё перечисленное выше + уставки ТА/полов, гистерезисы, калибровки, период насоса |
| **Секреты** | NVS | веб с паролем | Wi-Fi, пароль веб-интерфейса |

**Правила:**

1. **Защитные значения редактируются наравне с остальными.** Это решение владельца (AR-07): отладка таймингов «на живую» важнее аппаратного запрета.
2. **Валидация до применения, all-or-nothing.** `settings_validate.cpp` проверяет диапазон каждого поля и взаимные ограничения (`low < high`, `hysteresis > 0`, `TmaxHeat < TmaxTransfer`). Невалидный набор не применяется целиком.
3. **Версия + CRC32.** Несовпадение → дефолты из `limits.h` и запись в лог.
4. **Журнал изменений** (что/когда/старое→новое) — RAM ring + bounded NVS. Нужен, чтобы через месяц понять, что крутили.
5. **Применение — сразу после валидации.** `apply()` не блокируется состоянием FSM; в `Run`/`Ignition`/`Purge` в консоль и веб печатается предупреждение. Осознанный риск AR-07.
6. **Снимок, а не NVS в цикле.** `SettingsService` отдаёт `const Settings&` на тик; NVS в управляющем пути не читается.
7. **Сброс к дефолтам** — отдельное действие, возвращает значения из `limits.h`.
8. **Не safety-канал.** Настройки не меняют маски, `permit`, latch и логику supervisor — только числа, которые использует FSM.

### 9.4. Веб-интерфейс настройки (Wi-Fi)

- `platform/net` (Wi-Fi STA, при отсутствии сети — AP с паролем) + `services/web_config` (HTTP).
- **Отдельная задача; управляющий цикл никогда не ждёт сеть.** Бюджет ≤2 мс на тик, ответы — чанками, тяжёлые операции — вне `Control`.
- Доступ: пароль (не дефолтный), ограничение попыток входа, сессия с таймаутом, `POST` с CSRF-токеном.
- **Разрешено:** просмотр состояния/датчиков/настроек, правка **любых** настроек, включая защитные тайминги и лимиты (AR-07), `preview` → `apply`, сброс к дефолтам.
- **Запрещено:** снятие latch/reset, прямое управление выходами, запуск/останов горелки. Сброс latch — только локально (§7.4).
- **Не safety-канал:** зависание/отказ веб-сервиса не влияет на FSM, актуаторы, watchdog и связь с Nano. Веб-задача не входит в `SafetySupervisor` и не может выставить `permit`.
- До M6 сервис отсутствует в сборке (не заглушка с открытым портом): нет сети — нет сервера. Включение — отдельным изменением с ревью `safety-reviewer`.
- В `manual`/`bench-auto` веб-настройки допустимы, но **не** дают доступа к выходам: выходы — только консолью/кнопками и только в manual.

---

## 10. Профили сборки

### 10.1. PlatformIO

```ini
; Фрагмент: platform/board/monitor_speed сохраняются из текущего platformio.ini.
[platformio]
default_envs = esp32_core, arduino_safety   ; manual/native — ТОЛЬКО явный -e

[env]                       ; базовая секция: НАСЛЕДУЕТСЯ всеми env
build_flags = -I include

[env:esp32_core]            ; FIELD — единственная полевая сборка
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = ${env.build_flags}
  -I src_esp32 -Wall -Wextra -Wundef -std=gnu++17
  -DOILCTRL_TARGET_ESP32=1
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/>
  -<../src_esp32/manual/>
  -<../src_esp32/bench/>
  -<../src_esp32/app/app_manual.cpp>
  -<../src_esp32/app/app_bench_auto.cpp>
  -<../src_esp32/platform/host/>

[env:esp32_manual]          ; manual bypass: FSM/supervisor/transport ИСКЛЮЧЕНЫ
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = ${env.build_flags}
  -I src_esp32 -Wall -Wextra -Wundef -std=gnu++17
  -DOILCTRL_TARGET_ESP32_MANUAL=1
  -DOILCTRL_ENABLE_MANUAL_BYPASS=1
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/>
  -<../src_esp32/app/app_field.cpp>
  -<../src_esp32/app/app_bench_auto.cpp>
  -<../src_esp32/bench/>
  -<../src_esp32/domain/safety_supervisor.cpp>
  -<../src_esp32/domain/safety_policy.cpp>
  -<../src_esp32/domain/burner_fsm.cpp>
  -<../src_esp32/domain/interlocks.cpp>
  -<../src_esp32/domain/retry_policy.cpp>
  -<../src_esp32/services/safety_link.cpp>
  -<../src_esp32/hal/uart_link.cpp>
  -<../src_esp32/platform/host/>
  ; upload_port/monitor_port НЕ заданы: только явный --upload-port

[env:esp32_bench_auto]      ; FIELD-ядро + заглушка Nano (защит нет, AR-05)
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = ${env.build_flags}
  -I src_esp32 -Wall -Wextra -Wundef -std=gnu++17
  -DOILCTRL_TARGET_ESP32_BENCH_AUTO=1
  -DOILCTRL_ENABLE_NANO_STUB=1
  -DOILCTRL_ENABLE_AUTO=1
  -DOILCTRL_REQUIRE_CONTACTOR_FB=1
  -DOILCTRL_REQUIRE_KC868_POWER_FB=1
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/>
  -<../src_esp32/app/app_field.cpp>
  -<../src_esp32/app/app_manual.cpp>
  -<../src_esp32/manual/>
  -<../src_esp32/services/safety_link.cpp>
  -<../src_esp32/services/null_safety_source.cpp>
  -<../src_esp32/hal/uart_link.cpp>
  -<../src_esp32/platform/host/>
  ; upload_port/monitor_port НЕ заданы: только явный --upload-port

[env:arduino_safety]        ; собственные флаги, не наследует ESP32-настройки
platform = atmelavr
board = nanoatmega328
framework = arduino
monitor_speed = 115200
build_flags = -I include -Wall -Wextra
  -DOILCTRL_TARGET_AVR8=1
  -include include/build_profile.h
build_src_filter = +<../src_avr8/>

[env:native]                ; host-тесты FIELD
platform = native
test_build_src = yes
build_flags = ${env.build_flags}
  -I src_esp32 -DOILCTRL_TARGET_HOST=1 -Wall -Wextra -Wundef -std=gnu++17
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/domain/>
  +<../src_esp32/services/scheduler.cpp>
  +<../src_esp32/services/actuator_service.cpp>
  +<../src_esp32/platform/host/>

[env:native_manual]         ; host-тесты manual-ветки
platform = native
test_build_src = yes
build_flags = ${env.build_flags}
  -I src_esp32 -DOILCTRL_TARGET_HOST=1 -DOILCTRL_ENABLE_MANUAL_BYPASS=1
  -Wall -Wextra -Wundef -std=gnu++17
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/domain/>
  +<../src_esp32/services/scheduler.cpp>
  +<../src_esp32/services/actuator_service.cpp>
  +<../src_esp32/manual/>
  +<../src_esp32/platform/host/>

[env:native_bench_auto]     ; host-тесты bench-auto (заглушка lease)
platform = native
test_build_src = yes
build_flags = ${env.build_flags}
  -I src_esp32 -DOILCTRL_TARGET_HOST=1 -DOILCTRL_ENABLE_NANO_STUB=1
  -DOILCTRL_ENABLE_AUTO=1 -DOILCTRL_REQUIRE_CONTACTOR_FB=1 -DOILCTRL_REQUIRE_KC868_POWER_FB=1
  -Wall -Wextra -Wundef -std=gnu++17
  -include include/build_profile.h
build_src_filter =
  +<../src_esp32/domain/>
  +<../src_esp32/services/scheduler.cpp>
  +<../src_esp32/services/actuator_service.cpp>
  +<../src_esp32/bench/>
  +<../src_esp32/platform/host/>
```

- `include/build_profile.h` подключается **forced include** (`-include`) во **всех семи** env, включая `arduino_safety`, `esp32_bench_auto` и три native. Иначе guard-матрица не действует для AVR/host.
- **Guard-матрица** (активна во всех env):
  - target-макросы: `OILCTRL_TARGET_ESP32`, `OILCTRL_TARGET_ESP32_MANUAL`, `OILCTRL_TARGET_ESP32_BENCH_AUTO`, `OILCTRL_TARGET_AVR8`, `OILCTRL_TARGET_HOST`; допустимо **не более одного**;
  - `TARGET_ESP32*` взаимоисключающие между собой и с `TARGET_HOST`; `TARGET_AVR8 && (любой другой)` → `#error`;
  - `OILCTRL_ENABLE_MANUAL_BYPASS` допустим только при `TARGET_ESP32_MANUAL` или `TARGET_HOST`; иначе `#error`; `TARGET_ESP32_MANUAL && !ENABLE_MANUAL_BYPASS` → `#error`;
  - `OILCTRL_ENABLE_NANO_STUB` допустим только при `TARGET_ESP32_BENCH_AUTO` или `TARGET_HOST`; иначе `#error`; `TARGET_ESP32_BENCH_AUTO && !ENABLE_NANO_STUB` → `#error`;
  - `TARGET_ESP32_BENCH_AUTO && ENABLE_MANUAL_BYPASS` → `#error`; `TARGET_ESP32_MANUAL && ENABLE_NANO_STUB` → `#error`; `TARGET_ESP32 && (ENABLE_MANUAL_BYPASS || ENABLE_NANO_STUB)` → `#error`;
  - `TARGET_ESP32_BENCH_AUTO && !ENABLE_AUTO` → `#error` (bench-auto — это AUTO с другим источником lease);
  - ни одного target → FIELD (совместимость), реализуется через `#if !defined(...)` — **без** `#define` в заголовке;
  - `TARGET_ESP32_MANUAL`/`TARGET_ESP32_BENCH_AUTO` задаются **только** в своих env; CI-grep;
  - `OILCTRL_ENABLE_AUTO=1` — только для FIELD-сборки M4+ и bench-auto; `TARGET_ESP32_MANUAL && ENABLE_AUTO` → `#error`;
  - `ENABLE_AUTO && !OILCTRL_REQUIRE_CONTACTOR_FB` → `#error`; `ENABLE_AUTO && !OILCTRL_REQUIRE_KC868_POWER_FB` → `#error`; проверки действуют и для `TARGET_ESP32`, и для `TARGET_ESP32_BENCH_AUTO`: заглушка обязана выдавать `ON_CONFIRMED`/`PRESENT`, а не отключать требование;
  - оба `REQUIRE_*` — только `1`, не `0`; CI-grep по `platformio.ini`.
- `esp32_core` + manual-флаг **не проходит**: `PLATFORMIO_BUILD_FLAGS=-DOILCTRL_ENABLE_MANUAL_BYPASS=1 pio run -e esp32_core` → `#error`. У `pio run` нет собственного `-D`, поэтому используется переменная окружения. Отрицательные сборки в CI обязательны: manual-флаг в FIELD, `NANO_STUB` в FIELD, `ENABLE_AUTO` без `REQUIRE_CONTACTOR_FB`, bench-auto без `ENABLE_NANO_STUB`.
- Запрещено `#define OILCTRL_ENABLE_MANUAL_BYPASS`/`OILCTRL_TARGET_*` в любом `.h/.cpp` (кроме `build_profile.h`, где они только проверяются, не определяются); CI-grep.
- **FIELD — allow-list**: `+<../src_esp32/>` минус `manual/`, `bench/`, `app_manual.cpp`, `app_bench_auto.cpp`. Весь manual-only код обязан лежать в `manual/`, весь bench-only — в `bench/` (включая `supervisor_probe` — перенос из `hal/uart_probe.cpp`); новых исключений вне этих каталогов не добавлять. Исключения `app_manual.cpp`/`app_bench_auto.cpp` — единственные и зафиксированы в §4.
- **manual — явный список исключений**: supervisor/policy/FSM/interlocks/retry/link/uart_link не компилируются вовсе; «не инстанцируются» становится свойством сборки.
- **bench-auto — FIELD-ядро минус транспорт**: FSM/supervisor/interlocks/retry компилируются, `safety_link.cpp` и `uart_link.cpp` — нет; `NanoStubLink` заменяет источник lease. Попытка оставить оба источника — ошибка сборки (конфликт `ISafetySource` в composition root).
- `app_common.cpp` не содержит упоминаний supervisor/link/FSM/manual/bench (§4); проверяется `nm`.
- `native`/`native_manual`/`native_bench_auto`: только host-clean TU (`scheduler`, `actuator_service`, домен; для `native_bench_auto` — плюс `bench/`); `diagnostics`/`storage`/`safety_link` не входят (ESP-IDF/NVS-зависимости). `native_manual`/`native_bench_auto` компилируют весь `domain/`, поэтому **не являются** evidence изоляции сборок; изоляция доказывается только `esp32_manual`/`esp32_bench_auto` ELF.
- CI (двусторонне, с приложением вывода команд):
  - `nm -C esp32_core.elf` + `strings` — нет `ManualControl`, `ManualConsole`, `ManualMonitor`, `IManualActuatorPort`, `setManual`, `ActuatorService::tickManual`, `ExternalSupervisorProbe`, `SupervisorProbe`, `NanoStubLink`, `BenchAutoConsole`, `MANUAL BYPASS`, `NANO STUB`;
  - `grep -rn "autoMode\|tickManual" src_esp32/ --include=*.h --include=*.cpp` в FIELD-сборке: допустимы **только** объявление `tickManual` в `services/actuator_service.h` и комментарии; `autoMode` в коде отсутствует вовсе (в т.ч. `app/`);
  - `nm -C esp32_manual.elf` — нет `SafetySupervisor`, `SafetyLink`, `BurnerFsm`, `Interlocks`, `RetryPolicy`, `safetyPolicyEvaluate`;
  - `nm -C esp32_bench_auto.elf` — нет `SafetyLink`, `UartLink`, `NullSafetyLink`; `strings` содержит `NANO STUB`; `NanoStubLink` присутствует;
  - `nm -C` учитывает, что `IManualRequest`/`NullManualRequest` в FIELD **легальны**: проверяются точные имена, а не `grep -i manual`;
  - в build-dir `esp32_core` нет объектников из `manual/`, `bench/`, `app_manual`, `app_bench_auto`;
  - негативные сборки (см. выше) **падают**;
  - `default_envs` присутствует **только** в `[platformio]` и не содержит manual/bench-auto/native;
  - `pio test -e native`, `-e native_manual`, `-e native_bench_auto` проходят;
  - сборка `esp32_core`, `esp32_manual`, `esp32_bench_auto`, `arduino_safety`.
- `common_protocol.h` без `<Arduino.h>` — отдельное предложение с `safety-reviewer`.
- Стиль include: от корня `src_esp32` (`#include "domain/burner_fsm.h"`), относительные `../` запрещены.

### 10.2. Отладочная сборка `esp32_manual`

**Что это.** Сборка без `SafetySupervisor`, `SafetyLink`, FSM, interlocks и retry. Управление — прямыми командами по одному актуатору, с непрерывным чтением датчиков.

**Зачем.** Проверить реле, датчики, индикацию и поведение платы на котле, не дожидаясь полного safety-стека.

**Барьеров нет.** Нет `CONFIRM`, ограничения времени работы, детекта трафика UART, маркировки платы, журнала и обязательной перепрошивки после сеанса (AR-02…AR-04, AR-09). Сборка активна сразу после загрузки; в консоли — баннер `MANUAL BYPASS`.

**Что всё-таки остаётся (не процесс, а код):**
- deadman и max-on на опасную группу (§8.1) — без них одна забытая команда держит топливо открытым бесконечно;
- I2C fault → `outputsUnknown`, `allOff`, выход из режима, отказ команд;
- `build_src_filter`: в полевой сборке кода manual нет вообще.

**Про Nano.** На время manual его лучше физически отключить — не ради процедуры, а потому что подключённый Nano не получает heartbeat и через SR-B6 снимает питание платы, после чего PCF8574 даёт импульс реле (AR-12).

**Про безопасность.** Её здесь нет: ни давления воздуха, ни аварийной температуры масла, ни поплавков, ни safety-гейта. NC-клапаны закрыты без питания, и это всё (AR-10).

### 10.3. Отладка на котле

Отдельного стенда нет (AR-01). Отладочные сборки запускаются на реальной установке, с реальным маслом и горелкой (AR-11).

Практические следствия, которые полезно помнить, а не «выполнять как процедуру»:
- при включении питания возможен импульс реле (PCF8574 + ULN2003A) — топливо должно быть закрыто NC-клапанами;
- при отладке без Nano отсечки нет: останавливать нужно руками;
- ТЭН без термостата и термопредохранителя греть нельзя — это уже физика, а не код (AR-06);
- после отладки полезно прошить `esp32_core` и увидеть в баннере полевой профиль; если забыли — увидите по баннеру, когда откроете консоль (AR-02).

### 10.4. BENCH-AUTO — автоматика без Nano

**Что это.** Сборка `esp32_bench_auto`: полевое ядро (FSM, interlocks, supervisor, retry, diagnostics) с заглушкой вместо `SafetyLink`. Заглушка отдаёт `valid=true`, `permit=true`, `gateState=OpenSafe`, `sensorValid=kRequiredSensors`, `contactorFb=OnConfirmed`, `kc868PowerFb=Present`, `alarmBits=0` — «Nano говорит всё ОК».

**Отличие от v0.8:** нет `CONFIRM` и состояния deny. Заглушка активна с момента загрузки; в консоли баннер `NANO STUB — NO SAFETY SUPERVISOR`. Это AR-05: подтверждать нечего, потому что подтверждать некому.

**Что остаётся настоящим:** `SafetySupervisor`, инвариант §5.1, маски, latch, retry allow-list, `outputsUnknown`, watchdog, единый писатель. Проверяется логика Main Unit, а не безопасность.

**Что заглушка не умеет:** ломаться. Сценарии «permit lost», «link loss», «gate fault» проверяются host-тестами с фейковым lease, а не на железе. HIL-проверка потери связи требует реального Nano.

**Что не отменяется:** `ENABLE_NANO_STUB` + `ENABLE_MANUAL_BYPASS` → `#error`; заглушка живёт только в своей сборке и в FIELD не попадает (`nm`/`strings`).

---

## 11. Старт, останов, аварии

### 11.1. Порядок запуска

1. `platform::earlyApplicationSafe()` — direct GPIO OFF/pull-down до Serial/I2C.
2. Баннер: профиль, версия/хэш, reset reason, boot counter; для manual — `MANUAL BYPASS`, для bench-auto — `NANO STUB — NO SAFETY SUPERVISOR`.
3. Direct GPIO в OFF.
4. **PCF8574: до записи `0x00` состояние реле не контролируется.**
   - POR = I/O HIGH; ULN2003A инвертирует → катушка под током. Возможна активация всех реле.
   - «Boot-safe» не заявляется. Нужны измерение POR/reset/brownout (BLK-012) и схемная митигация (`schematic-engineer`). До этого реальные нагрузки не подключаются.
5. I2C init (GPIO 4/15), проверка PCF8574, запись OFF + read-back; I2C dead at boot → `Fault`/лог, без reboot-loop.
6. Датчики — без блокирующих ожиданий.
7. FIELD: `NullSafetyLink` до M3b. Bench-auto: `NanoStubLink` сразу отдаёт allow. Manual: supervisor/link/FSM не создаются.
8. Manual-only проверка перед первым включением нагрузки: I2C healthy, `!outputsUnknown`, read-back всех выходов == `Off`. Не выполнилось — команды отклоняются, в лог причина. Bench-auto: заглушка активна с загрузки, баннер обязателен.
9. `mode = BootSafe` → самопроверка → `Off`.
10. `loop()`.

**Важно:** шаг 5 не гарантирует OFF до записи в PCF8574, а reset MCU не сбрасывает выходной latch PCF8574. Гарантированное выключение нагрузки — только физическое снятие питания (автомат/кнопка аварийного отключения).

### 11.2. Поведение

| Событие | AUTO (FIELD) | Manual Bypass |
|---|---|---|
| Потеря lease после валидного | danger OFF, latch, `Fault`→postpurge→`Latched` | не применимо |
| `valid=false` с старта | `Off/Deny`, AUTO недостижим | не применимо |
| `I2C_FAULT`/read-back mismatch | danger OFF, bounded retry, latch | `outputsUnknown`, `allOff`, **принудительный выход из Manual**, отказ команд, требование снять питание нагрузок |
| Board-off/on Nano | импульс на PCF8574 недопустим; осциллограф | **недопустимо подключать Nano**; при трафике UART — выход из Manual |
| Overrun/зависание | safe-группа → TWDT reset → safe boot | `allOff` → TWDT reset; гарантии OFF после reset нет |
| Невалидный датчик | отказ перехода | лог/отображение, команды не блокируются |
| Сброс/reboot | latch переживает (M4+); AUTO по reset | `RunMode` → `Off`; latch PCF8574 **не** сбрасывается; manual не возобновляется |
| Deadman/max-on | не применимо | опасная группа OFF по таймеру; лог |
| `PumpBoiler` при аварии | OFF по умолчанию; run-on — BLK-007 | прямое управление (процессная группа) |

---

## 12. Поэтапная реализация

Этапы — это порядок работы, а не ворота с согласованиями. «Готово» означает «работает и проверено глазами», а не «подписан акт».

| Этап | Содержание | Готово, когда | Nano |
|---|---|---|---|
| **M0. Каркас** — ГОТОВО | `config/limits.h` + `settings.h`, scheduler, `ActuatorService`, `ModeManager`, `SafetyPolicy`, `ISafetySource` + `NullSafetySource`, доменные типы lease, каталоги `manual/` и `bench/`, host-тесты | собирается, тесты проходят, полевой и отладочные бинарники не смешиваются (`nm`) | не нужен |
| **M1. Manual на котле** | 6 реле PCF8574 + direct-GPIO «SSR pilot», read-back, I2C health, мониторинг датчиков | реле щёлкают как ожидалось, датчики видны, `all off`/deadman работают | отключён |
| **M1b. ТЭН** | SSR + внешний термостат + термопредохранитель | реальный ТЭН греет и отключается по термостату | отключён |
| **M2. Датчики** | DS18B20, MAX6675, flame baseline, валидность/фильтры | значения правдоподобны, невалидные распознаются | наблюдение |
| **M3a. Link TX** | UART-транспорт, heartbeat TX, loopback | Nano видит кадры, timeout детектится | N3a: приём |
| **M3b. Link RX** | lease, `everValid/permit`, latch | потеря lease ведёт к снятию разрешений | N3b: permit/gate TX |
| **M4. AUTO FSM** | FSM, interlocks, retry allow-list, flame/exhaust, NVS latch/счётчики | розжиг и горение идут по циклу, авария ведёт в `Fault→Latched` | обязателен |
| **M4b. BENCH-AUTO** | заглушка `NanoStubLink`, баннер, консоль | FSM крутится без Nano, `nm` не находит транспорт в bench-elf | физически отсутствует |
| **M5. Plant** | ТА, heat demand, гистерезисы, насос котла | котёл включается и выключается по температуре ТА | обязателен |
| **M6. Настройки и веб** | `SettingsService`, веб по Wi-Fi, RS485, детект 220 В | настройки меняются из браузера, управление не блокируется | не влияет |

**Что нужно, чтобы полевая прошивка считалась полевой:** `esp32_core` + прошитый Nano + смонтированные NC-клапаны, контактор, автомат, термопредохранитель. Всё остальное — по желанию, список в `blockers.md` («Проверки перед финальной сборкой»).

### 12.1. Итог M0 (2026-11-13)

Сделано ровно то, что в строке M0, без расширения области:

| Проверка | Результат |
|---|---|
| `pio test -e native` / `native_manual` / `native_bench_auto` | все наборы PASS: 51 / 64 / 52 теста (policy 17, actuator 18, mode_manager 9, scheduler 5, manual 14, bench 2 — по профилю env) |
| `pio run -e esp32_core` / `esp32_manual` / `esp32_bench_auto` / `arduino_safety` | все SUCCESS, Flash 22.4–22.7 % (ESP32), 4.9 % (Nano) |
| `nm` по elf | `esp32_core`: только `safetyPolicyEvaluate`, `SafetySupervisor`, `NullSafetySource`; `esp32_manual`: `tickManual`/`ManualControlService`, supervisor отсутствует; `esp32_bench_auto`: `NanoStubLink` + supervisor, `NullSafetySource` отсутствует |
| объектники в `.pio/build/esp32_core` | ни одного из `manual/`, `bench/`, `app_manual.cpp`, `app_bench_auto.cpp` |
| `strings` | «MANUAL BYPASS» только в manual, «NANO STUB» только в bench-auto |
| guard-матрица (негативные сборки) | падают: manual-флаг в FIELD, `NANO_STUB` в FIELD, `ENABLE_AUTO` без `REQUIRE_*`, bench-auto без `NANO_STUB`, две цели сразу, флаг `=0`, manual+AUTO |

Отклонения от документа, зафиксированные по факту:

- `SafetySupervisor` реализован сразу в M0 (тонкая stateful-обёртка `ever*`), а не заглушкой: тесты §7.3 без него не проверить.
- `NullSafetySource` вместо `NullSafetyLink` — источник lease по факту сервис, а не транспорт; `SafetyLink`/`UartLink` появятся в M3.
- `platform/nvs` сделан на ESP-IDF `<nvs.h>`, а не на `Preferences.h`: LDF не подхватывает библиотеку при `build_src_filter` из `../src_esp32/`.
- Функция `bit(ActuatorId)` переименована в `actuatorBit()`: `Arduino.h` определяет макрос `bit()`, и в manual-сборке имя конфликтует.
- `src/.gitkeep` — PlatformIO требует существующий `src/`, иначе фильтры `+<../src_esp32/>` не работают.
- `src_avr8/main.cpp` временно не включает `common_protocol.h`: AVR-часть `static_assert` в драфте протокола не проходит (BLK-006, M3/N3).

**Следующий шаг — M1 (manual на котле):** PCF8574 (6 реле) + direct-GPIO «SSR pilot», read-back, I2C health, мониторинг датчиков. Код M0 к этому готов; нужны только конкретные линии из `pinmap.md` (DRAFT) и проверка на плате.


## 13. Что можно заглушать, а что — никогда

| Элемент | Manual Bypass | FIELD (M0–M3) | FIELD (AUTO, M4+) |
|---|---|---|---|
| Safety lease/permit | не используется | `NullSafetyLink` → deny | реальный lease |
| `SafetySupervisor` | **не создаётся** | не требуется для AUTO без M3 | обязателен |
| `SEN_FLAME` | мониторинг, не блокирует | `Invalid` → AUTO запрещён | утверждённый датчик |
| `SEN_AIR_PRESSURE` | **недоступен** (прямой вход Nano) → `NotFitted` | только lease Nano | только lease Nano |
| `SEN_OIL_TEMP_SAFETY` | **недоступен** (прямой вход Nano) → `NotFitted` | только lease Nano | только lease Nano |
| `SEN_FLOAT_MIN/MAX` | **недоступны** (прямой вход Nano) → `NotFitted` | только lease Nano | только lease Nano |
| Технологические DS18B20 | `NotFitted` отображается | `NotFitted` → переход запрещён | обязательны, где FSM требует |
| Boot-safe direct GPIO | **нельзя** | **нельзя** | **нельзя** |
| Boot-safe PCF8574 | не заявляется | измерение+митигация | то же |
| Latch/no auto-resume | не применимо (manual) | **нельзя** в AUTO | **нельзя** |
| Watchdog | **нельзя** | **нельзя** | **нельзя** |
| Единственный писатель | **нельзя** | **нельзя** | **нельзя** |
| Manual-путь | **только по дефайну** | **отсутствует** | **отсутствует** |
| Safety-клапаны | **нельзя** | **нельзя** | **нельзя** |

**Bench-auto** — это колонка `FIELD (AUTO, M4+)` с одним отличием: `SafetyLink` заменён на `NanoStubLink`. Всё остальное (supervisor, инвариант §5.1, latch, retry, watchdog, единый писатель) действует без изменений; заглушается **только источник lease**. Строка «Safety lease/permit» для bench-auto читается как «заглушка → allow сразу после загрузки».

---

## 14. Тестирование

Тесты — это способ не сломать то, что уже работает, а не формальный акт. Пропускать их можно, но тогда поломки ищутся на котле.

1. **Host-тесты (`native`, Unity):**
   - `SafetyPolicy`/`SafetySupervisor` (AUTO): lease, alarms, gate, feedback, age;
   - FSM: все переходы, `PostPurge`, `Fault→Latched`;
   - **инвариант §5.1** по `decision.next`: `Fault/PostPurge/Shutdown/Latched` и `PrePurge` ⇒ `(demand & kSafetyOffGroup) == 0` (включая `AirValveMain` и `HeaterOil`); `Init` ⇒ `kAllOff`; `Idle` ⇒ нет fuel/spark/air, `HeaterOil` только при `permitHeater && allowHeatOil`; `Run` ⇒ oil OPEN, spark OFF, `HeaterOil` только при `permitHeater && allowHeatOil`; `AirValveMain` открыт только в `Ignition`/`Run`; «лишний» бит снимается и поднимает `Internal`; manual использует `tickManual`, инвариант к нему не применяется;
   - `ActuatorDemand{}` ⇒ `bits=kAllOff`, `permitDangerous=false`, `permitHeater=false` (fail-closed дефолт);
   - **FIELD-изоляция manual-пути:** `nm -C esp32_core.elf` не содержит `tickManual`; host-тест FIELD не может вызвать `tickManual` (линковка падает); `grep` по `src_esp32/` не находит `autoMode`;
   - **`permitHeater`:** `Idle + permitProcess + contactor ON_CONFIRMED + power PRESENT + kValidOilTempSafety + kValidFloatMin` ⇒ `HeaterOil` ON; `gate CLOSED` не запрещает ТЭН; `!permitDangerous` **не** снимает ТЭН;
   - **ТЭН fail-closed:** `!kValidFloatMin` ⇒ `HeaterOil` OFF (не греть «на сухую»); `kc868PowerFb ∈ {Unknown, Absent, Fault}` при `requireKc868PowerFb=true` ⇒ OFF; невалидная `SEN_OIL_TEMP_MAIN` ⇒ OFF;
   - **ТЭН в `Run`:** `Run + permitHeater + canHeatOil` ⇒ ON; `!permitHeater`/`!allowHeatOil` ⇒ OFF; `Ignition`/`PrePurge`/`PostPurge`/`Fault`/`Latched` ⇒ OFF **всегда**; fault в `Run` снимает ТЭН вместе с топливом; инвариант `Run` (§5.1) не допускает `PumpOilTransfer`/`IgnitionUnit` и требует fan+air;
   - **bench-auto (`native_bench_auto`):** `NanoStubLink` сразу даёт `permitDangerous`/`permitHeater` по тем же правилам, что реальный lease; заглушка не проходит в FIELD (проверка сборки, не host-теста);
   - **настройки:** невалидный набор отклоняется целиком; CRC/version mismatch ⇒ дефолты; `apply()` применяет **любые** поля, включая защитные тайминги и лимиты (AR-07), в том числе в `Run`/`Ignition` — только печатает предупреждение; сброс к дефолтам возвращает значения из `limits.h`;
   - **защитные константы из веба:** после `apply()` нового `Tprepurge` FSM использует новое значение на следующем тике; взаимные ограничения (`TpostpurgeFail ≥ Tpostpurge`, `TlatchMin > 0`, `low < high`) проверяются до применения; попытка записать значение вне диапазона из `limits.h` отклоняется; `Settings` не может изменить маски, `permit`, latch и логику supervisor;
   - **продувка:** `PrePurge` с `kPurgePathApproved=false` по истечении `Tprepurge` ⇒ `Fault`, не `Ignition`;
   - **`everPermitted_`:** `permit` был, затем пропал ⇒ `Latch` (не `Allow`);
   - **`ignitionAttempts`:** дефолт до чтения NVS = `kMaxIgnitionAttempts` ⇒ `IgnitionTimeout` = `Forbidden`;
   - **Домен:** полярность `GateState::OpenSafe`/`ContactorFb::OnConfirmed` проверяется `static_assert` в host-TU;
   - **retry allow-list**: `None` → `Forbidden`; `FlameLoss` → `Forbidden`; неизвестная причина → `Forbidden`; `IgnitionTimeout` при лимите → `Forbidden`; `ignitionAttempts` write-ahead до `Ignition`, сброс на `reset`/штатном останове/`Tstable`; NVS corrupt → retry запрещён; rate-limit при неизвестном времени → запрещён; `stableRunReached` сбрасывается на `PrePurge`; retryable-fault не даёт `Latch` supervisor'а;
   - `ActuatorService`: две точки входа `tickAuto`/`tickManual` с общим `apply()`, маска AUTO, readback, `outputsUnknown` (в т.ч. AUTO: `actBits` не выглядит подтверждённым OFF);
   - `ModeManager`: `Manual` в enum всегда; с фейком `IManualRequest(false)` переход `Off→Manual` невозможен; `reset` снимает latch и очищает `ignitionAttempts`, rate-limit — нет; reboot → `Off`; `demand` при `Fault` остаётся постпродувкой до `BurnerState::Latched`;
   - `newHeatEdge`: повторный пуск после `HeatDone`/`UserOff`/`TrunMax` только по новому фронту; `stopRequest` → `PostPurge`/`Idle`; `UserOff` не выставляет latch;
   - wrap-safe время.
2. **Host-тесты (`native_manual`):**
   - `ManualControlService`: per-actuator `setManual`, `allOff`, deadman, max-on, min-off;
   - **deadman**: `status`/`sensors` каждые 5 с **не** продлевают; команда другого актуатора не продлевает; `set fan 1` без действий ⇒ OFF на max-on (тик, не только команда);
   - `set fan 1` + `set oil 1` ⇒ fan остаётся ON (нет второго вектора);
   - `set heater 1` в manual разрешён без `canHeatOil`; `set x 1` после автo-OFF стартует новое окно, `set x 1` при включённом `x` окно не сбрасывает;
   - I2C fault ⇒ `outputsUnknown`, выход из Manual, отказ команд;
   - `ManualMonitor`: snapshot/quality без записи в `ActuatorService`;
   - вход при `outputsUnknown` ⇒ отказ команд; привязки к плате нет — проверяется отсутствие `BENCH_ID`/`IBenchIdentity`/`IManualRuntimeBudget`/`IExternalSupervisorProbe` в коде.
3. **HIL (на котле):** осциллограф/мультиметр — POR/reset/brownout, board-off, поведение выходов, latch PCF8574 после reset. Это не «стенд», а обычная проверка на месте.
4. **Fault-injection:** обрыв/КЗ, I2C NACK, overrun, reset, sensor invalid в manual (команды не блокируются).
5. **Arch-check (CI):** `digitalWrite`/I2C/`Serial` только в `hal/`/`platform/`; `nm -C` в обе стороны; `OILCTRL_ENABLE_MANUAL_BYPASS`/`OILCTRL_ENABLE_NANO_STUB` + `esp32_core` → ошибка (через `PLATFORMIO_BUILD_FLAGS`); в FIELD-build нет объектников `manual/`, `bench/`; строки `MANUAL BYPASS`/`NANO STUB` отсутствуют в FIELD; сборка всех env; `pio test -e native`, `-e native_manual`, `-e native_bench_auto`. `native_manual`/`native_bench_auto` **не** доказывают изоляцию — только `esp32_manual`/`esp32_bench_auto` ELF.
6. **Traceability:** boot-safe, no-permit, latch, retry allow-list, manual isolation, single writer, «Fault ⇒ danger OFF».

---

## 15. Правила кодирования

- C++17, `enum class`, `constexpr`, `static_assert`; без исключений/RTTI в управляющем пути.
- Без `String`, `std::function`, `std::string`, `std::vector`, `new` после init в критичном пути.
- `-Wall -Wextra -Wundef`; bounded-буферы; статические массивы.
- `#if OILCTRL_*` — **только** в `include/build_profile.h` (forced include) и composition root; в `domain/`, `services/` и общих заголовках профильных веток нет (ADR-11).
- Include — от корня `src_esp32` (`#include "domain/burner_fsm.h"`); относительные `../` запрещены.
- Время — `uint32_t` + wrap-safe; никаких `delay()`.
- Все выходы — через `ActuatorService` (`tickAuto` в AUTO, `tickManual` в manual-сборке, общий `apply()`); manual — только через `IManualActuatorPort`.
- `Serial`/`Wire`/`SPI` — только в `hal/` и `platform/`; прикладной код работает через порты.
- Safety-флаги — безопасный дефолт; ошибки централизованно.
- Safety-дефолты и диапазоны — `constexpr` в `config/limits.h`; текущие значения (включая защитные) — в `Settings` (AR-07).
- Любые настройки — только через `config/settings.h` + `SettingsService`; в других файлах «своих» настроечных чисел нет.
- Веб/сеть — только `platform/net` + `services/web_config`, вне управляющего цикла; прикладной код не вызывает HTTP/Wi-Fi напрямую.
- `RunMode` не сериализуется в NVS/протокол как число (только строковое имя).
- Имена кода — английские; комментарии/документация — русские.

---

## 16. Решения и неизвестности

**Решено (не обсуждается, записано в `docs/safety/accepted_risks.md`):**

| Вопрос | Решение |
|---|---|
| Отдельный стенд для отладки | Нет. Отладка на котле (AR-01) |
| Привязка к плате, маркировка, процедура перепрошивки | Нет. Видно по баннеру (AR-02) |
| Подтверждение входа в отладочный режим | Нет. Сборка активна сразу (AR-03) |
| Ограничение времени работы manual | Нет. Счётчика и NVS-бюджета нет (AR-04) |
| Заглушка Nano в bench-auto | Да, активна с загрузки (AR-05) |
| ТЭН в горении | Разрешён по `permitHeater && canHeatOil`, без compile-time гейта (AR-06) |
| Защитные константы | Хранятся в `Settings`, правятся через веб с паролем (AR-07) |
| Обязательные ревью и блокеры | Нет (AR-08) |
| Журнал и письменные процедуры | Нет (AR-09) |

**Остальные вопросы — рабочие, решаются по ходу:**

- **Manual guards:** max-on/min-off и deadman 10 с на опасную группу (§8.1) — остаются, потому что без них одна забытая команда держит топливо открытым. Отключение — флагами `ALLOW_NO_MAXON`/`ALLOW_NO_DEADMAN`.
- **`FlameLoss` retry:** запрещён. Потеря пламени — всегда latch.
- **Retry `IgnitionTimeout`:** старт `kMaxIgnitionAttempts=1` (retry выключен), потом 3 — после того как тайминги подобраны на котле.
- **`actBits`:** `applied` + `actValid`/`outputsUnknown`; `uart.md`/`common_protocol.h` («requested») привести к этому. `attemptNo`/`retryCount` — там же.
- **FSM-тайминги:** `Tprepurge/Tign/TignOilDelay/Tpostpurge/TpostpurgeFail/TrunMax/Tstable` — дефолты в `limits.h`, дальше подбираются через веб.
- **`common_protocol.h` без `<Arduino.h>`** — нужно для host-маппинга protocol→domain enum.
- **`HeaterOil`** — отдельная `kHeaterGroup`, входит в `kSafetyOffGroup`; в `Ignition`/`PrePurge`/`PostPurge`/`Fault`/`Latched` — OFF.
- **Fan-only продувка:** `AirValveMain` открыт только в `Ignition`/`Run`; `kPurgePathApproved` гейтит только `PrePurge → Ignition`. Пока флаг `false`, AUTO-розжиг недостижим — это намеренно.
- **`newHeatEdge`** — внутри `BurnerFsm`; `UserOff` — через `stopRequest`/`IStopInput`.
- **`NullManualRequest`** — в `app/`, `IManualRequest` опрашивает `app/`; `IManualActuatorPort` реализует `ManualControlService`.
- **Flame baseline direct GPIO:** нужна CANDIDATE-линия под SR-B13/S-47 (сейчас в `pinmap.md` только ADC). Добавить и пересчитать бюджет линий.
- **`ALARM_MANUAL_LATCH`** (бит 10): удалить или описать сценарий.
- **Nano-workstream:** N3a/N3b (link TX/RX на Nano) как отдельные этапы; M3b/M4 зависят от них.
- **`OILCTRL_ENABLE_AUTO`:** M4-сборка требует `OILCTRL_REQUIRE_CONTACTOR_FB=1` и `OILCTRL_REQUIRE_KC868_POWER_FB=1`, иначе `#error`.
- **`ignitionAttempts` fail-closed:** дефолт поля — `kMaxIgnitionAttempts` (не `0`); до чтения NVS retry запрещён.
- **Доменные типы lease:** `GateState`/`ContactorFb`/`PowerFb`/`AlarmBits`/`SensorValidBits` — в `domain/safety_types.h`; маппинг protocol→domain только в `SafetyLink` со `static_assert`.
- **Build-guard:** `OILCTRL_TARGET_ESP32`/`_ESP32_MANUAL`/`_ESP32_BENCH_AUTO`/`_AVR8`/`_HOST`, guard во всех env, `default_envs` только в `[platformio]`.
- **`ISafetySource`:** интерфейс источника lease в `services/safety_source.h`; FIELD — `SafetyLink`, bench-auto — `NanoStubLink`.

---

## 17. Риски и ограничения

**Принятые риски живут в одном месте — `docs/safety/accepted_risks.md` (AR-01…AR-12).** Здесь только технические ограничения реализации, которые полезно помнить при кодировании. Ни один пункт не является блокером.

- **Manual-лимиты:** актуаторы держатся до max-on/deadman (опасная группа — `Tign+2 с` / `TrunMax+10 с` / `TmaxHeat+30 с`; deadman 10 с продлевается `keep`). Процессная группа (fan/pump) — до 30 мин без deadman. Независимой защиты в manual нет.
- **Нет привязки к плате:** отладочный бинарник технически можно оставить на котле (AR-02). Увидеть режим можно только по баннеру.
- **bench-auto:** заглушка Nano не измеряет давление, аварийную температуру масла и поплавки, не открывает safety-гейт (AR-05). Домен не отличает заглушку от реального lease — различие только в сборке и баннере.
- **Manual + подключённый Nano:** Nano не получает heartbeat → SR-B6 power-cycle → POR PCF8574 → импульс реле (AR-12). Поэтому Nano на время manual лучше отключить физически.
- **PCF8574/I2C не fail-safe:** POR может активировать реле; reset MCU **не сбрасывает** latch; программно не устраняется.
- **I2C fault в manual:** `allOff` может не дойти; состояние выходов неизвестно, режим завершается, питание нагрузок снимается руками.
- **Retry розжига:** каждая неудачная попытка подаёт дозу топлива; ограничения — `TpostpurgeFail ≥ Tpostpurge`, лимит попыток, rate limit, write-ahead NVS.
- **Retry и latch зависят от протокола:** без `attemptNo`/`retryCount` retry не реализуется; без согласованной процедуры снятия supervisor latch возможен deadlock `F_FlameLoss`.
- **Ранний AUTO без датчиков** — deny-by-default и `NotFitted`.
- **Reboot-авто-возобновление:** без NVS latch (M4) reset может «забыть» аварию; до M4 AUTO недостижим.
- **I2C dead at boot/reboot-loop:** bounded recovery, без reboot-loop.
- **NVS blocking:** запись после safe, bounded; счётчики retry персистятся write-ahead.
- **Alarm fatigue:** latch + rate limit; неизвестные alarm-биты — latch.
- **ТЭН подогрева (`HeaterOil`)** — 220 В внутри ёмкости с маслом. В manual он управляется напрямую, без `canHeatOil` и без Nano-датчика `SEN_OIL_TEMP_SAFETY`; ограничение — max-on/min-off/deadman и внешний термостат/термопредохранитель. В `Run` — дополнительный источник тепла рядом с горелкой; аппаратного ограничителя на Main Unit нет, защита — `SEN_OIL_TEMP_MAIN`, валидность `kValidOilTempSafety` и внешний термостат.
- **Fan-only продувка:** достаточность продувки одним вентилятором при закрытом `AirValveMain` — открытый вопрос; пока `kPurgePathApproved=false`, AUTO-розжиг недостижим.
- **Протокол ↔ домен:** `SafetyLease.gateState/alarmBits/contactorFb` — «сырые» числовые поля; маппинг обязан жить в `SafetyLink`, со `static_assert`. Иначе полярность `gateState` может быть прочитана наоборот (fail-open).
- **CANDIDATE pinmap:** M1/M2 используют DRAFT-линии; при отладке это допустимо, при финализации — сверить с платой.
- **Документ не заменяет** pinmap/schematic.

---

## 18. Итог

Модульный монолит с одним управляющим циклом, чистым доменом, единым писателем актуаторов и **тремя** сборками:
- **FIELD** (`esp32_core`) — AUTO, deny-by-default, lease/permit, latch, retry allow-list. Это единственная сборка, где безопасность обеспечивается полностью;
- **MANUAL** (`esp32_manual`) — без `SafetySupervisor`/`SafetyLink`/FSM, прямое управление по одному актуатору, непрерывный мониторинг датчиков, max-on/deadman. Без барьеров и подтверждений (AR-02…AR-04);
- **BENCH-AUTO** (`esp32_bench_auto`) — FIELD-ядро с заглушкой Nano вместо `SafetyLink`; защита отсутствует, риск AR-05.

M0–M1 дают ручное управление на котле; M2–M4 добавляют датчики, связь и AUTO FSM; M4b — bench-auto для отладки без Nano; M6 — единые настройки и веб. Manual- и bench-код отсутствуют в FIELD; FIELD-код не может включить ни manual, ни заглушку; `Fault`/`PostPurge` всегда означают опасную группу OFF.

---

## 19. Что стоит привести в согласие

Документы ниже разойдутся с этим, если их не поправить. Обязательных согласований нет — это просто список того, что нужно обновить, чтобы через месяц не искать противоречия.

| Документ | Что поправить |
|---|---|
| `docs/architecture/blockers.md` | уже переведён в «Открытые технические вопросы»; BLK-013 закрыт принятием риска |
| `docs/safety/accepted_risks.md` | источник правды по рискам; дополнять при новых осознанных решениях |
| `docs/safety/checklist.md` | смягчить формулировки: памятка, не gate; убрать runtime-бюджет, `CONFIRM`, маркировку, журнал |
| `AGENTS.md` | уже обновлён (риск-регистр, ревью не gate, M0 готов, текущий шаг M1) |
| `docs/architecture/modules.md` | убрать обязательность ревью; отладочные сборки — не Main Unit; нет привязки к плате |
| `docs/hardware/periph_distribution.md` | убрать «стендовое питание» и bench-only исключения — стенда нет |
| `docs/protocol/uart.md` | `actBits` = `applied` + `actValid`; `attemptNo`/`retryCount`; manual не участник протокола |
| `docs/hardware/pinmap.md` | DRAFT; добавить CANDIDATE direct-GPIO под baseline `SEN_FLAME`; CANDIDATE-линии при отладке допустимы |
| `include/common_protocol.h` | `actBits` «requested» → `applied`; `attemptNo`/`retryCount`; судьба `ALARM_MANUAL_LATCH`; без `<Arduino.h>` |
| `docs/safety/bench_procedure.md` | **не создаётся**: стенда нет (AR-01, AR-09) |
| `docs/safety/blk002_closure_package.md` | убрать оговорку про стендовое питание — её больше нет |
| `docs/safety/fsm_safety_boundary.md` | согласовать `ExhaustInvalid`/`SensorInvalid`/`Timeout` → hard latch с §7.5 |
| `rnd/peripherals.md` | настройки и веб — программная функция, новых линий не требуют |

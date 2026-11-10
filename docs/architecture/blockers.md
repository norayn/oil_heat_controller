# Блокеры и approval gates

Статус: **ACTIVE**. Часть блокеров уточнена решениями пользователя; выполнены док-closures (BLK-003 doc ready/ACK requested, BLK-011 checklist expanded, BLK-002: не самозамыкающийся Nano cutoff, без шунтирования; **draft принятия остаточного риска залипания — ACK required**). Схемные/safety-валидационные условия остаются. Добавлен **BLK-012** (board verification A6).
Дата обновления: 2026-11-10.
Область: `docs/architecture/*`, `docs/hardware/*`, `docs/protocol/uart.md`, `include/common_protocol.h`, будущие pinmap/schematic, `src_esp32/`, `src_avr8/`.

> **Правило:** до закрытия CRITICAL/HIGH условий запрещены: финализация pinmap/schematic как «утверждено», реализация safety-протокола в прошивке без `safety-reviewer` APPROVE, ослабление Nano cutoffs/gates/block-stop. Любая реализация — только с `safety-reviewer` (и `schematic-engineer` для силовой части/изоляции/220 В). **Номиналы средств защиты/автоматов/контакторов выбираются на отдельном шаге** и не закрывают safety-анализ топологии/feedback/fail-safe.

> **Детали по темам:** BLK-002 — `docs/safety/blk002_closure_package.md`; BLK-003 — `docs/safety/fsm_safety_boundary.md`; распределение периферии и Nano backup по актуаторам — `docs/hardware/periph_distribution.md`; review-гейт — `docs/safety/checklist.md`.

## Сводка

| ID | Severity | Статус | Тема | Блокирует |
|---|---|---|---|---|
| BLK-001 | MEDIUM | ACCEPTED DESIGN (SCHEMATIC OPEN) | `ACT_FAN_BLOWER` — KC868 relay; Nano может обесточить **всю плату KC868** (SR-B6, coarse) + автомат. **Per-circuit Nano cutoff не требуется** (решение пользователя). Топливная безопасность — safety-gates + `SEN_AIR_PRESSURE` + обесточивание ТЭН/подкачки/розжига контактором. | schematic SR-B6/board-off side effects, подтверждение автомата/цепи, `safety-reviewer` APPROVE |
| BLK-002 | CRITICAL | DESIGN GATE | Общий Nano-контактор / **силовое реле (не самозамыкающееся, только Nano)** для ТЭН/подкачки/розжига: сварка/залипание/несостоявшееся размыкание, single-point failure, feedback/fail-safe outputs. Финальная тепловая защита ТЭН — **термовыключатель/«теплушка» на корпусе бака**. | schematic, SR-B4/SR-B18 доказательство, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-003 | HIGH | ACCEPTED RISK (ACK REQUESTED, DOC READY) | `SEN_THERMOCOUPLE_K`/температура выхлопа — зона обратной связи Main Unit, **не зона контроля Nano Safety**. Граница/поведение FSM — `docs/safety/fsm_safety_boundary.md`. | `safety-reviewer` **ACK**; пересмотр при изменении требований |
| BLK-004 | HIGH | PARTIALLY SPECIFIED (SCHEMATIC OPEN) | Per-actuator Nano hardware backup: board-off Nano, общий контактор, физическое дублирование клапанов, циркуляционный насос. | schematic, actuator wiring, code review |
| BLK-005 | HIGH | PROPOSED / NOT VALIDATED | `SEN_FLAME` = фоторезистор на **аналоговый вход KC868** + экранирование + ЭМС-тесты; fallback — опторазвязка. Отклонение от SR-B13/S-47. | schematic, noise/ignition tests, протокол flame/ignition supervision, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-006 | HIGH | IN PROGRESS (DOC DRAFT, SAFETY-REVIEW REQUIRED) | Миграция `include/common_protocol.h`/`docs/protocol/uart.md` (safety-oriented draft). | любой safety UART код, heartbeat/permit, совместимость |
| BLK-007 | MEDIUM | POLICY SPECIFIED (SCHEMATIC OPEN) | `ACT_PUMP_BOILER`: KC868 relay + возможное (не по умолчанию) дублированное включение Nano (автомат). Оставание ON в аварии — не проблема (решение пользователя). | pinmap/schematic, управление насосом котла |
| BLK-008 | MEDIUM | OPEN | SR-B6 supervised power-cycle KC868 (whole-board power-off — mitigation в BLK-001/004): BOR/Hi-Z/feedback/weld/ramp tests не трассированы. | schematic, Nano power-control, стендовые тесты |
| BLK-009 | MEDIUM | OPEN | Grounding/isolation §7.18 и 220 В монтаж/транзиенты (S-48) не выбраны/не подтверждены. | schematic, PCB/wiring, UART/feedback isolation, EMC/safety tests |
| BLK-010 | MEDIUM | OPEN | RS485 S-49 не финализирован. | RS485 design, slave specs |
| BLK-011 | LOW | PARTIALLY CLOSED (DOC EXPANDED) | `docs/safety/checklist.md` расширен (BLK-001..012 gates). Дополнять при появлении новых схемных/испытательных артефактов. | будущие safety-review / evidence review |
| BLK-012 | HIGH | OPEN (BOARD VERIFICATION REQUIRED) | **KC868-A6 v1.3**: подтвердить ревизию vs официальная схема A6, MCU/flash/PSRAM, USB/CH340C, свободные GPIO/strapping, реле (SPDT/ULN2003A) и PCF8574 при reset/brownout/power loss, опто-входы EL357N, ADC A1–A4, RS485 (MAX13487EESA, без изоляции), питание/бросок. | `schematic-engineer` + `safety-reviewer`; до финализации pinmap A6 и сборки/прошивки |

## Детали блокеров

### BLK-001 (MEDIUM → ACCEPTED DESIGN, SCHEMATIC OPEN) — Вентилятор наддува

**Решение пользователя:** `ACT_FAN_BLOWER` управляется через реле на плате KC868. Nano может **обесточить всю плату KC868** и тем самым отключить реле; плюс защитный автомат. Продолжение работы вентилятора в аварии **не считается критическим** (электрозащита автоматом; продувка тёплым воздухом не safety-critical).

**Интерпретация:** независимый per-circuit Nano cutoff вентилятора **не требуется**. Топливная/горелочная безопасность — **Nano safety-gates** (топливо/воздух), **`SEN_AIR_PRESSURE`** на Nano (нет давления => топливо не подаётся) и обесточивание `ACT_HEATER_OIL`/`ACT_PUMP_OIL_TRANSFER`/`ACT_IGNITION_UNIT` общим Nano-контактором.

**Остаточные условия (схемные/режимные):**
- [ ] Подтвердить защитный автомат/электрическую защиту цепи вентилятора 220 В (топология/селективность/монтаж, S-48; номиналы — отдельный шаг).
- [ ] SR-B6 board-off (если используется): Nano **никогда** не режет своё питание; только питание KC868; supervised/last-resort, current-limited, no fast cycling, min off-time/limits, 3-state feedback, latch, fail-safe outputs. Board-off убивает Main Unit/FSM/heartbeat; **не использовать как штатный механизм управляемой постпродувки**. Состояние реле/нагрузок при board-off — подтвердить схемой.
- [ ] При пропадании питания KC868 реле/выходы платы ведут себя предсказуемо (подтвердить схемой/поведением).
- [ ] `SEN_AIR_PRESSURE` на Nano — обязательный инвариант (прямой вход).
- [ ] `safety-reviewer` APPROVE после схемного подтверждения.

### BLK-002 (CRITICAL → DESIGN GATE) — Общий Nano-контактор / силовое реле

**Решение пользователя:** не архитектурный запрет, а выбор конечного устройства коммутации. Номиналы — отдельный шаг. **Контактор/силовое реле не самозамыкающееся, без самоподхвата, управляется только Nano; без шунтирования/дублирующих путей питания группы в обход Nano cutoff.** Для ТЭН — **термовыключатель/«теплушка» на корпусе нагреваемого бака**.

**Draft (REQUIRES `safety-reviewer` ACK, НЕ утверждено):** остаточный риск залипания/сварки/несостоявшейся отсечки принят как остаточный; отдельная аппаратная weld-mitigation в текущем объёме не требуется. Риск **не устранён физически**.

**Компенсирующие меры (обязательны, не снимать):**
- Nano safety-gates топлива/воздуха (NC, только Nano) + `SEN_AIR_PRESSURE` предотвращают опасную подачу топлива/воздуха даже при сварке общего контакта.
- `ACT_HEATER_OIL` (SR-B4): SSR не аварийное отключение; cutoff последовательно с ТЭН; теплушка + термопредохранитель/предохранители. **Тип сброса теплушки (ручной/самовозврат) — фиксация + `safety-reviewer` ACK**, совместимость с latch/no auto-resume.
- `ACT_PUMP_OIL_TRANSFER` (SR-B18): при сварке контакта насос может остаться запитанным; опасная подача топлива в горелку ограничивается safety-gates/уровнями/Nano-логикой. Последствия зафиксировать в схеме/FMEA.
- `ACT_IGNITION_UNIT`: при сварке контакта блок розжига может остаться запитанным; опасная подача топлива/воздуха блокируется safety-gates; последствия — FMEA.
- **Feedback fail-safe полярность** (`UNKNOWN/FAULT` != «отсечено»), **latch/no restart** при неподтверждённой отсечке — обязательны.

**Пакет закрытия, схема, FMEA, evidence, критерий закрытия:** `docs/safety/blk002_closure_package.md`. **Без него BLK-002 не закрывается.**

### BLK-003 (HIGH → ACCEPTED RISK, ACK REQUESTED) — Температура выхлопа

`SEN_THERMOCOUPLE_K`/MAX6675 — **не зона контроля Nano Safety**; обратная связь процесса горения Main Unit. Nano **не обязан** иметь независимый hardware-триггер перегрева выхлопа. Защита опирается на Nano safety-gates, `SEN_AIR_PRESSURE`, прямые Nano датчики, heartbeat/FSM supervision, общий Nano-контактор, latch/fail-safe.

- [x] Граница зафиксирована: `docs/safety/fsm_safety_boundary.md`.
- [x] Требование к Main Unit: при недостоверности/timeout/fault — безопасный прикладный режим.
- [ ] **Требуется `safety-reviewer` ACK** границы/принятого риска.
- [ ] Placement/интерфейс MAX6675 — по схеме (шум, SPI, диагностика fault), без требования Nano safety-path.

Пересмотр обязателен при изменении требований к защите от перегрева выхлопа, компоновки/топки/материалов или появлении независимого датчика на Nano.

### BLK-004 (HIGH → PARTIALLY SPECIFIED, SCHEMATIC OPEN) — Per-actuator Nano hardware backup

**Решение пользователя:**
- Подача воздуха — управление через PCF8574 relay; Nano может обесточить всю плату KC868 (coarse mitigation, BLK-001/SR-B6). Safety-gate `ACT_AIR_VALVE_SAFETY` (физическое дублирование/последовательность клапанов) — обязательный инвариант.
- Нагрев масла, насос подкачки, розжиг — отключаются через общий Nano-контактор (BLK-002).
- Клапаны дублированы физически (`*_VALVE_MAIN` + `*_VALVE_SAFETY` NC последовательно).
- Циркуляционный насос котла — возможное дублированное управление с Nano (BLK-007).

**Таблица «актуатор → прикладное управление → независимый Nano path → fail-safe состояние»:** `docs/hardware/periph_distribution.md`. **Требуется схемная фиксация и запрет обхода; программный backup запрещён.**

### BLK-005 (HIGH → PROPOSED/NOT VALIDATED, DEVIATION RISK) — `SEN_FLAME`

**Решение пользователя:** фоторезистор на **аналоговый вход KC868**, экранированный кабель + практические тесты наводок при розжиге; при недостаточности — опторазвязка.

**Safety-уточнение:** это **отклонение** от SR-B13/S-47 («только direct ESP32 GPIO + внешний гальванически изолированный формирователь digital/Schmitt, fail-safe полярность/диагностика; запрещено KC868 opto/PCF8574/I2C/ADC-raw»). Аналоговый путь чувствителен к наводкам ВВ розжига, дрейфу, делителю/питанию, ошибкам ПО/порога. **Не утверждать без валидации.**

**Требуется закрыть/валидировать:**
- [ ] Схема входа: питание/делитель/фильтр/защита, экранирование, routing от ВВ розжига/220 В; обрыв/КЗ/потеря питания датчика = **«нет пламени»**.
- [ ] Алгоритм: пороги с гистерезисом, debounce/интегрирование, явная диагностика `invalid/saturated/stuck`, таймауты розжига/потери пламени, latch/no auto-restart.
- [ ] Практические тесты ЭМС/наводок **во время розжига** и при всех режимах вентилятора/контакторов; критерии PASS/FAIL.
- [ ] Если наводки/нестабильность — **обязательный переход** на опторазвязку/изолированный формирователь к **direct ESP32 GPIO**.
- [ ] Протокол/heartbeat несёт достаточный flame/ignition/timeout статус для Nano (BLK-006).
- [ ] `safety-reviewer` + `schematic-engineer` APPROVE. До валидации — **PROPOSED, not safety-approved**.

### BLK-006 (HIGH → IN PROGRESS, SAFETY-REVIEW REQUIRED) — Миграция протокола

**Выполнено в docs (draft):** `include/common_protocol.h` — safety-oriented черновик (`HeartbeatMainToSafety`/`HeartbeatSafetyToMain`, `protoVer/seq/CRC`, FSM/I2C/flame/permit/gate/latch/sensorValid поля) с пометкой **SAFETY DRAFT — DO NOT USE IN FIRMWARE UNTIL `safety-reviewer` APPROVE**. `docs/protocol/uart.md` синхронизируется при финализации.

**Осталось:**
- [ ] `safety-reviewer` APPROVE формата и семантики (CRC algorithm/coverage/seed/polynomial, endian/packing, baud/timeout, seq window/stale/duplicate/resync, unknown/reserved/`protoVer`, test vectors).
- [ ] Синхронизировать `docs/protocol/uart.md` и `include/common_protocol.h`.
- [ ] Реализация в `src_esp32/` и `src_avr8/` — отдельные изменения, каждое с `safety-reviewer`.

### BLK-007 (MEDIUM → POLICY SPECIFIED, SCHEMATIC OPEN) — `ACT_PUMP_BOILER`

**Решение пользователя:** насос котла управляется через PCF8574 relay; может быть (не по умолчанию) дублированно включён напрямую Nano (электрическая защита автоматом). Оставание включённым в аварии — **не проблема**.

**Требуется закрыть:** схема (какой контур включает Nano, приоритеты, защита автоматом, обратное питание/диоды/изоляция при параллельном/альтернативном включении), поведение при отказе датчиков/связи, монтаж 220 В (S-48). Номиналы автомата — отдельный шаг.

### BLK-012 (HIGH, OPEN) — KC868-A6 board-level verification

**Контекст:** Main Unit — **KC868-A6, ревизия v1.3** (подтверждено 2026-11-09), ESP32-WROOM-32, USB-C + CH340C. Официальная схема A6 доступна, но **не снимает** сверку с фактической ревизией и board test.

**Требуется закрыть (evidence, `schematic-engineer` + `safety-reviewer`):**
- [x] **Ревизия платы: v1.3** (2026-11-09).
- [ ] Сверить применимость `KC868-A6-schematic.pdf` к ревизии **v1.3** (визуально/BOM); маркировка ESP32-WROOM-32, flash/PSRAM.
- [ ] Реле: SPDT (`PCOM/NC/NO`), драйвер ULN2003A; состояние при **POR** (до init I2C), при **reset/WDT/brownout/power loss**; PCF8574 latch.
- [ ] Опто-входы EL357N: порог/ток, «сухой контакт» vs внешнее питание.
- [ ] ADC A1–A4: диапазон/точность/шум; A3/A4 — 0–5 В vs 4–20 мА.
- [ ] Свободные GPIO/strapping/boot (0/2/5/12/15; 34–39 input-only), включая линии, освобождаемые отказом от RS232 и LoRa/nRF.
- [ ] RS485 MAX13487EESA — **без гальваноизоляции**; терминирование/TVS; только non-safety.
- [ ] Питание 12 В: ток холостой/с реле/Wi-Fi, бросок при board-off/on (SR-B6).
- [ ] `SEN_FLAME` (BLK-005): подтвердить ADC A6 + ЭМС-тесты, либо fallback SR-B13/S-47.

**Не закрывает:** BLK-002/003/005/006/007/008/009/010 и внешние силовые/защитные цепи. **Pinmap A6 остаётся DRAFT.**

## Обязательные approval gates

| Gate | Кто | Когда |
|---|---|---|
| Док-правки safety-логики/распределения/протокола (включая `include/`) | `safety-reviewer` | после каждого значимого изменения |
| Pinmap + силовая/изоляционная схема | `schematic-engineer` + `safety-reviewer` | до шага 3 финализации и до сборки/прошивки |
| BLK-001 blower + SR-B6 board-off side effects | `schematic-engineer` + `safety-reviewer` | до схемного подтверждения цепи вентилятора/автомата/SR-B6 |
| BLK-002 общий контактор (топология/feedback/weld/остаточный риск) | `schematic-engineer` + `safety-reviewer` | до утверждения силовой схемы |
| BLK-003 exhaust temp accepted risk | `safety-reviewer` ACK | до финализации FSM/safety spec |
| BLK-005 flame sensor (ADC+photoresistor / opto fallback) | `schematic-engineer` + `safety-reviewer` | до утверждения схемы и прошивки логики пламени |
| BLK-006 protocol migration | `safety-reviewer` | до любого safety UART кода в прошивке |
| BLK-012 A6 board verification | `schematic-engineer` + `safety-reviewer` | до финализации pinmap A6 и до сборки/прошивки |
| Стендовые тесты (A6 §7, noise/ignition BLK-005, SR-B6 ramp/weld/feedback) | `safety-reviewer` evidence review | до опытной эксплуатации |

> **KC868-A6:** официальная схема доступна (`KC868-A6-schematic.pdf`), но требует сверки с ревизией. Board-level факты (контакты/драйвер реле, reset/brownout, опто-входы, A3/A4, RS485 без изоляции, питание) подтверждаются **схемой + измерением/даташитами** (`schematic-engineer`), а safety-функции строятся **только внешним слоем Nano**. PCF8574/I2C — **не fail-safe path**. Pinmap-статус «утверждён» требует board test + approvals.

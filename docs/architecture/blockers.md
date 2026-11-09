# Блокеры и проблемы архитектуры (текущее состояние)

Статус: **ACTIVE** — часть блокеров уточнена решениями пользователя (2026-11-08); выполнены быстрые док-closures (BLK-003 doc ready/ACK requested, BLK-011 checklist expanded, BLK-002: не самозамыкающийся Nano cutoff, без шунтирования; **draft принятия остаточного риска залипания — ACK required**). Схемные/safety-валидационные условия остаются. **2026-11-09: Main Unit переведён на KC868-A6 (не v3); добавлен BLK-012 (A6 board verification).**  
Дата обновления: 2026-11-09  
Источник: safety-review `safety-reviewer` (sessions `ses_e457842baffeXg77w4V8gW6k71`, `ses_e45392268ffeARiw1nwAAJ2q8y`, `ses_e45110436ffeDWddhh0k0qOgZr`, verdicts **APPROVE_WITH_CONDITIONS**, docs-only/delta) + решения пользователя по BLK-001…007, BLK-002 non-self-latching clarification.  
Область: `docs/architecture/*`, `docs/hardware/*`, `docs/protocol/uart.md`, `include/common_protocol.h`, будущие pinmap/schematic, `src_esp32/`, `src_avr8/`.

> **Правило:** до закрытия CRITICAL/HIGH условий запрещены: финализация pinmap/schematic как «утверждено», реализация safety-протокола в прошивке без `safety-reviewer` APPROVE, ослабление Nano cutoffs/gates/block-stop. Любая реализация — только с `safety-reviewer` (и `schematic-engineer` для силовой части/изоляции/220В). **Номиналы средств защиты/автоматов/контакторов выбираются на отдельном шаге** (не закрывает safety-анализ топологии/feedback/fail-safe).

## Сводка

| ID | Severity | Статус | Тема | Блокирует |
|---|---|---|---|---|
| BLK-001 | MEDIUM | ACCEPTED DESIGN (SCHEMATIC OPEN) | `ACT_FAN_BLOWER` управляется KC868 relay; Nano может обесточить **всю плату KC868** (SR-B6, coarse) + защитный автомат. **Независимый per-circuit Nano cutoff вентилятора не требуется** по решению пользователя: продолжение работы вентилятора в аварии не считается критичным (электрозащита автоматом; продувка тёплым воздухом — не safety-critical). Топливная безопасность обеспечивается Nano safety-gates + `SEN_AIR_PRESSURE` + обесточиванием ТЭН/подкачки/розжига контактором. | schematic SR-B6/board-off side effects, подтверждение автомата/цепи, `safety-reviewer` APPROVE |
| BLK-002 | CRITICAL | DESIGN GATE (не «закрыт выбором компонента») | Общий Nano-контактор / **силовое реле (не самозамыкающееся, управляется только Nano)** для ТЭН/подкачки/розжига: сварка/залипание/несостоявшееся размыкание, single-point failure, feedback/fail-safe outputs. Для `ACT_HEATER_OIL` финальная тепловая защита — **термовыключатель/«теплушка» на корпусе нагреваемого бака** (в силовой/защитной цепи по схеме). Выбор устройств/номиналов — отдельно; safety-требования обязательны. | schematic, SR-B4/SR-B18 доказательство, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-003 | HIGH | ACCEPTED RISK (ACK REQUESTED, DOC READY) | `SEN_THERMOCOUPLE_K`/температура выхлопа — зона обратной связи процесса горения Main Unit, **не зона контроля Nano Safety**. Nano не имеет независимого триггера перегрева выхлопа. Граница/обязательное поведение FSM зафиксированы в `docs/safety/fsm_safety_boundary.md`. | `safety-reviewer` **ACK** по `docs/safety/fsm_safety_boundary.md`; при изменении требований — пересмотр |
| BLK-004 | HIGH | PARTIALLY SPECIFIED (SCHEMATIC OPEN) | Per-actuator Nano hardware backup уточнён пользователем (KC868 board power-off Nano, общий контактор, физическое дублирование клапанов, цирк. насос). Нужна схемная фиксация и запрет обхода. | schematic, actuator wiring, code review |
| BLK-005 | HIGH | PROPOSED / NOT VALIDATED (DEVIATION RISK) | `SEN_FLAME` = фоторезистор на **аналоговый вход KC868**, экранированный кабель + тесты наводок при розжиге; при недостаточности — опторазвязка. Отклонение от SR-B13/S-47 «direct ESP32 GPIO + внешний изолированный формирователь digital/Schmitt; запрещено ADC-raw/PCF8574/opto-I2C path». | schematic, noise/ignition tests, протокол flame/ignition supervision, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-006 | HIGH | IN PROGRESS (DOC DRAFT, SAFETY-REVIEW REQUIRED) | Миграция `include/common_protocol.h`/`docs/protocol/uart.md` начата (safety-oriented draft). Требует `safety-reviewer` APPROVE до реализации в прошивке. | любой safety UART код, heartbeat/permit, совместимость |
| BLK-007 | MEDIUM | POLICY SPECIFIED (SCHEMATIC OPEN) | `ACT_PUMP_BOILER`: KC868 relay + возможное дублированное включение Nano (автомат). Оставание включённым в аварии — не проблема по решению пользователя. Нужна схемная фиксация/защита. | pinmap/schematic, управление насосом котла |
| BLK-008 | MEDIUM | OPEN | SR-B6 supervised power-cycle KC868 (whole-board power-off используется как mitigation в BLK-001/004): BOR/Hi-Z/feedback/weld/ramp tests не трассированы. | schematic, Nano power-control, стендовые тесты |
| BLK-009 | MEDIUM | OPEN | Grounding/isolation §7.18 и 220В монтаж/транзиенты (S-48) не выбраны/не подтверждены. | schematic, PCB/wiring, UART/feedback isolation, EMC/safety tests |
| BLK-010 | MEDIUM | OPEN | RS485 S-49 не финализирован. | RS485 design, slave specs |
| BLK-011 | LOW | PARTIALLY CLOSED (DOC EXPANDED) | `docs/safety/checklist.md` расширен (BLK-001..010 gates, теплушка/тип сброса, BLK-005 validation/fallback, BLK-006 test vectors/gates, contactor feedback fail-safe, SR-B6). Дополнять при появлении новых схемных/испытательных артефактов. | будущие safety-review / evidence review |
| BLK-012 (BLK-A6-HW) | HIGH | OPEN (BOARD VERIFICATION REQUIRED) | Переход Main Unit на **KC868-A6 (не v3, ESP32-WROOM-32)**: подтвердить ревизию vs официальная схема A6, MCU/flash/PSRAM, USB/CH340C, свободные GPIO/strapping, реле (SPDT/ULN2003A) и PCF8574 поведение при reset/brownout/power loss, опто-входы EL357N, ADC A1–A4 (A3/A4 0–5 В vs 4–20 мА), RS485 (MAX13487EESA, без изоляции), питание/бросок. | `schematic-engineer` + `safety-reviewer`; до финализации pinmap A6 и сборки/прошивки |

## Детали блокеров и решений

### BLK-001 (MEDIUM → ACCEPTED DESIGN, SCHEMATIC OPEN) — Вентилятор наддува
**Решение пользователя:** `ACT_FAN_BLOWER` управляется через реле на плате KC868. Nano может **обесточить всю плату KC868** и тем самым отключить реле; плюс защитный автомат. Продолжение работы вентилятора в аварийном состоянии **не считается критической проблемой** (электрически защищён автоматом; продувка тёплым воздухом через котёл неприятна, но не критична).  
**Интерпретация:** независимый per-circuit Nano cutoff вентилятора **не требуется**. Топливная/горелочная безопасность обеспечивается **Nano safety-gates** (топливо/воздух), **`SEN_AIR_PRESSURE`** на Nano (нет/недостаточно давления => топливо не подаётся), и обесточиванием `ACT_HEATER_OIL`/`ACT_PUMP_OIL_TRANSFER`/`ACT_IGNITION_UNIT` общим Nano-контактором/силовым реле (не самозамыкающимся, только Nano). Coarse board-off KC868 (SR-B6) остаётся аварийной возможностью.  
**Остаточные условия (схемные/режимные):**
- [ ] Подтвердить защитный автомат/электрическую защиту цепи вентилятора 220В (топология/селективность/монтаж, S-48; номиналы — отдельный шаг).
- [ ] SR-B6 board-off (если используется): Nano **никогда** не режет своё питание; только питание KC868; supervised/last-resort, current-limited, no fast cycling, min off-time/limits, 3-state feedback, latch, fail-safe outputs (SR-B14/B15/SR-B12), weld/stuck handling. Board-off убивает Main Unit/FSM/heartbeat; **не использовать как штатный механизм управляемой постпродувки**. При board-off состояние реле KC868/прикладных нагрузок (в т.ч. вентилятора) должно быть подтверждено схемой как fail-safe/предсказуемое.
- [ ] При пропадании питания KC868 реле/выходы платы должны вести себя предсказуемо (подтвердить схемой/поведением при потере питания).
- [ ] `SEN_AIR_PRESSURE` на Nano — обязательный инвариант (прямой вход Nano).
- [ ] `safety-reviewer` APPROVE после схемного подтверждения.

### BLK-002 (CRITICAL → DESIGN GATE) — Общий Nano-контактор / силовое реле (ТЭН/подкачка/розжиг)
**Решение пользователя:** не блокер как архитектурный запрет, а вопрос выбора конечного устройства, обеспечивающего коммутацию. Номиналы — отдельный шаг. **Контактор/силовое реле не самозамыкающееся; по сути мощное реле, управляемое Nano** (без схемы самоподхвата/самоблокировки; обесточивание/отпускание Nano => fail-safe размыкание по схеме). **Нет шунтирования/дублирующих путей питания группы в обход Nano cutoff.** Для финальной защиты ТЭН подключается **термовыключатель/«теплушка» на корпусе нагреваемого бака**.  
**Draft: принятие остаточного риска залипания/сварки/несостоявшейся отсечки (REQUIRES `safety-reviewer` ACK, НЕ утверждено):** пользователь принимает этот риск как остаточный и не требует отдельной аппаратной weld-mitigation в текущем объёме. Это **не устраняет** физическую возможность отказа. До ACK компенсирующие меры и схемное подтверждение обязательны.
**Компенсирующие меры (обязательны, не снимать):**
- Nano **safety-gates** топлива/воздуха (NC, только Nano) + `SEN_AIR_PRESSURE` (прямой Nano) предотвращают опасную подачу топлива/воздуха даже при сварке общего контакта.
- `ACT_HEATER_OIL` (SR-B4): SSR не аварийное отключение; cutoff последовательно с ТЭН; **теплушка на корпусе нагреваемого бака** + термопредохранитель/предохранители по подтверждённой схеме. При сварке контакта неконтролируемый нагрев ограничивается только этой термозащитой. **Тип сброса теплушки (ручной/самовозврат) — фиксация + `safety-reviewer` ACK**, совместимость с latch/no auto-resume.
- `ACT_PUMP_OIL_TRANSFER` (SR-B18): при сварке контакта насос может остаться запитанным, но подача топлива в горелку ограничивается safety-gates/уровнями/Nano-логикой; последствия «питание осталось при закрытых gates» зафиксировать в схеме/FMEA.
- `ACT_IGNITION_UNIT`: при сварке контакта блок розжига может остаться запитанным; опасная подача топлива/воздуха должна блокироваться safety-gates; последствия зафиксировать в схеме/FMEA.
- **Feedback fail-safe полярность** (`UNKNOWN/FAULT` != «отсечено»), **latch/no restart** при неподтверждённой/неуспешной отсечке — обязательны.
- **Не самозамыкающийся, без шунтирования/самоподхвата**, fail-safe размыкание (SR-B14/B15/SR-B12/S-42) — подтверждается схемой.
**Пакет закрытия (чек-лист/FMEA/evidence):** `docs/safety/blk002_closure_package.md`  
**Требуется закрыть (схема):**
- [ ] Цепь ТЭН (SR-B4): внешний SSR/драйвер (не аварийное отключение) + **общий Nano-контактор/силовое реле (не самозамыкающееся, только Nano, без шунтирования/самоподхвата)** последовательно с ТЭН + **термовыключатель на корпусе нагреваемого бака** + термопредохранитель/предохранители. Тип сброса теплушки — фиксация + `safety-reviewer` ACK.
- [ ] Подкачка (SR-B18): hardware block/stop fail-safe OFF, отсутствие шунтирования/самоподхвата.
- [ ] Контактор/реле: feedback fail-safe полярности, поведение при reset/brownout/Hi-Z/WDT, latch/no restart. Остаточный риск залипания/сварки — явная запись + `safety-reviewer` ACK + подтверждённые компенсирующие меры.
- [ ] `schematic-engineer` + `safety-reviewer` APPROVE (схема + risk acceptance draft).

### BLK-003 (HIGH → ACCEPTED RISK, ACK REQUESTED / DOC READY) — Температура выхлопа
**Решение пользователя:** температура выхлопа (`SEN_THERMOCOUPLE_K`/MAX6675, если используется) — **не зона контроля модуля безопасности Nano**; основная задача — обратная связь по процессу горения (Main Unit).  
**Интерпретация:** Nano **не обязан** иметь независимый hardware-триггер по перегреву выхлопа. Защита от опасных режимов опирается на Nano safety-gates, `SEN_AIR_PRESSURE`, прямые Nano датчики (`SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_*`), heartbeat/FSM supervision, общий Nano-контактор/силовое реле (не самозамыкающееся, только Nano; ТЭН/подкачка/розжиг), latch/fail-safe.  
**Документ для ACK:** `docs/safety/fsm_safety_boundary.md` (граница ответственности, Nano-инварианты, обязательное поведение Main Unit при invalid/timeout/fault, условия пересмотра).
**Остаточные условия:**
- [x] Явно зафиксирована граница: перегрев выхлопа не является независимым Nano safety interlock (`docs/safety/fsm_safety_boundary.md`).
- [x] Зафиксировано требование к Main Unit: при недостоверности/timeout/fault/invalid безопасный прикладный режим, запрет продолжения опасного розжига/работы без валидной обратной связи там, где это требует FSM.
- [ ] **Требуется `safety-reviewer` ACK** принятого риска/границы ответственности по `docs/safety/fsm_safety_boundary.md`. При изменении требований к защите от перегрева выхлопа — обязательный пересмотр.
- [ ] Placement/интерфейс MAX6675 на Main Unit — по схеме (шум, SPI, диагностика fault), но без требования Nano safety-path.

### BLK-004 (HIGH → PARTIALLY SPECIFIED, SCHEMATIC OPEN) — Per-actuator Nano hardware backup
**Решение пользователя (уточнение):**
- Подача воздуха — управление через PCF8574 relay; Nano может обесточить всю плату KC868 (и реле) — coarse mitigation (см. BLK-001/SR-B6). Плюс Nano safety-gate `ACT_AIR_VALVE_SAFETY` (физическое дублирование/последовательность клапанов) остаётся обязательным инвариантом.
- Нагрев масла, насос подкачки, розжиг — отключаются через общий Nano-контактор/силовое реле (не самозамыкающееся, только Nano).
- Клапаны дублированы физически (`*_VALVE_MAIN` + `*_VALVE_SAFETY` NC последовательно).
- Циркуляционный насос котла (`ACT_PUMP_BOILER`) — дублированное управление с Nano (см. BLK-007).

| Актуатор/группа | Прикладное управление | Независимый Nano hardware path / mitigation | Fail-safe состояние | Статус |
|---|---|---|---|---|
| `ACT_OIL_VALVE_MAIN` | KC868/PCF8574 relay | **`ACT_OIL_VALVE_SAFETY` NC, только Nano**, последовательно (физ. дублирование) | Safety-gate закрыт | Инвариант |
| `ACT_AIR_VALVE_MAIN` | KC868/PCF8574 relay | **`ACT_AIR_VALVE_SAFETY` NC, только Nano**, последовательно + Nano может обесточить плату KC868 (coarse, SR-B6) | Safety-gate закрыт; board-off аварийно | Инвариант + BLK-001/SR-B6 |
| `ACT_HEATER_OIL` | KC868/PCF8574 relay / SSR pilot | **Общий Nano-контактор/реле (не самозамыкающийся, только Nano)** (SR-B4) последовательно с ТЭН + **термовыключатель/«теплушка» на корпусе нагреваемого бака** + термопредохранитель/предохранители | Обесточен/отключён при размыкании защитной цепи | BLK-002 gate |
| `ACT_PUMP_OIL_TRANSFER` | KC868/PCF8574 relay | **Общий Nano-контактор/реле (не самозамыкающийся, только Nano)** (SR-B18) | OFF/обесточен | BLK-002 gate |
| `ACT_IGNITION_UNIT` | KC868/PCF8574 relay | **Общий Nano-контактор/реле (не самозамыкающийся, только Nano)** | Обесточен | BLK-002 gate |
| `ACT_FAN_BLOWER` | KC868/PCF8574 relay | Nano может обесточить **всю плату KC868** (coarse, SR-B6) + автомат; **per-circuit Nano cutoff не требуется** (принятое решение) | Электрозащита автоматом; топливная безопасность — safety-gates/`SEN_AIR_PRESSURE`/контактор ТЭН+подкачка+розжиг | BLK-001 **ACCEPTED DESIGN**, schematic open |
| `ACT_PUMP_BOILER` | KC868/PCF8574 relay | Дублированное включение Nano напрямую возможно (схема); автомат. Оставание включённым в аварии — не проблема (решение пользователя) | По схеме; не safety-critical stop | BLK-007 |
| `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_*` | RS485 slave | Slave watchdog/fail-safe defaults (S-49) | Безопасная позиция / OFF | BLK-010 |

### BLK-005 (HIGH → PROPOSED/NOT VALIDATED, DEVIATION RISK) — `SEN_FLAME`
**Решение пользователя:** фоторезистор, заводится на **аналоговый вход KC868**, экранированным кабелем + тесты на практике достаточна ли защита от наводок при розжиге. Если недостаточно — реализация с опторазвязкой.  
**Safety-уточнение:** это **отклонение** от ранее зафиксированного условия SR-B13/S-47 («только direct ESP32 GPIO + внешний гальванически изолированный формирователь digital/Schmitt, fail-safe полярность/диагностика; запрещено KC868 opto/PCF8574/I2C/ADC-raw»). Аналоговый путь через KC868/ADC более чувствителен к наводкам ВВ розжига, дрейфу, делителю/питанию, ошибкам ПО/порога. **Не утверждать без валидации.**  
**Требуется закрыть/валидировать:**
- [ ] Схема входа: источник питания/делитель/фильтр/защита, экранирование, routing от ВВ розжига/220В; поведение при обрыве/КЗ/потере питания датчика = **«нет пламени»** (fail-safe).
- [ ] Алгоритм: пороги с гистерезисом, debounce/интегрирование, явная диагностика `invalid/saturated/stuck`, таймауты розжига/потери пламени, latch/no auto-restart.
- [ ] Практические тесты ЭМС/наводок **во время розжига** и при всех режимах вентилятора/контакторов; критерии PASS/FAIL зафиксировать.
- [ ] Если наводки/нестабильность — **обязательный переход** на опторазвязку/внешний изолированный формирователь (digital/Schmitt) к **direct ESP32 GPIO** (предпочтительный fail-safe вариант).
- [ ] Протокол/heartbeat должен нести достаточный flame/ignition/timeout статус для Nano супервизии (BLK-006).
- [ ] `safety-reviewer` + `schematic-engineer` APPROVE. До валидации статус: **PROPOSED, not safety-approved**.

### BLK-006 (HIGH → IN PROGRESS, SAFETY-REVIEW REQUIRED) — Миграция протокола
**Решение пользователя:** провести необходимую миграцию.  
**Выполнено в docs (draft):** `include/common_protocol.h` заменён на safety-oriented черновик пакетов/констант (`HeartbeatMainToSafety`, `HeartbeatSafetyToMain`, `protoVer/seq/CRC`, FSM/I2C/flame/permit/gate/latch/sensorValid поля) с пометкой **SAFETY DRAFT — DO NOT USE IN FIRMWARE UNTIL `safety-reviewer` APPROVE**; `docs/protocol/uart.md` должен быть синхронизирован при финализации.  
**Осталось:**
- [ ] `safety-reviewer` APPROVE формата и семантики.
- [ ] Синхронизировать `docs/protocol/uart.md` и `include/common_protocol.h` (имена полей, размеры, CRC, endian/packing, baud/timeout значения).
- [ ] Реализация в `src_esp32/` и `src_avr8/` — отдельные изменения, каждое с `safety-reviewer`.

### BLK-007 (MEDIUM → POLICY SPECIFIED, SCHEMATIC OPEN) — `ACT_PUMP_BOILER`
**Решение пользователя:** насос котла управляется через PCF8574 relay; также может быть дублированно включён напрямую Nano (электрическая защита автоматом). Если останется включённым в аварийной ситуации — это не является проблемой.  
**Требуется закрыть:** схема (какой контур включает Nano, приоритеты, защита автоматом, обратное питание/диоды/изоляция при параллельном/альтернативном включении), поведение при отказе датчиков/связи, монтаж 220В (S-48). Номиналы автомата — отдельный шаг.

### BLK-012 (HIGH, OPEN) — KC868-A6 board-level verification
**Контекст:** Main Unit переведён на **KC868-A6 (не v3)**, ESP32-WROOM-32, USB-C + CH340C. Официальная схема A6 доступна, но **не снимает** необходимость сверки с фактической ревизией и board test. **Фактическая плата: ревизия v1.3** (подтверждено пользователем 2026-11-09).  
**Требуется закрыть (evidence, `schematic-engineer` + `safety-reviewer`):**
- [x] **Ревизия платы подтверждена: v1.3** (2026-11-09).
- [ ] Сверить применимость `KC868-A6-schematic.pdf` именно к ревизии **v1.3** (визуально/BOM); маркировка ESP32-WROOM-32, flash/PSRAM.
- [ ] Реле: SPDT (`PCOM/NC/NO`), драйвер ULN2003A; состояние при **POR** (до init I2C), при **reset/WDT/brownout/power loss**; PCF8574 latch.
- [ ] Опто-входы EL357N: порог/ток, «сухой контакт» vs внешнее питание.
- [ ] ADC A1–A4: диапазон/точность/шум; A3/A4 — 0–5 В vs 4–20 мА.
- [ ] Свободные GPIO и strapping/boot (0/2/5/12/15; 34–39 input-only).
- [ ] RS485 MAX13487EESA — **без гальваноизоляции**; терминирование/TVS; только non-safety.
- [ ] Питание 12 В: ток холостой/с реле/Wi-Fi, бросок при board-off/on (SR-B6).
- [ ] `SEN_FLAME` (BLK-005): переназначить на подтверждённые ADC A6 + ЭМС-тесты, либо fallback SR-B13/S-47.
**Не закрывает:** BLK-002/003/005/006/007/008/009/010 и внешние силовые/защитные цепи — они проектируются отдельно. **Pinmap A6 остаётся DRAFT.**

## Обязательные approval gates

| Gate | Кто | Когда |
|---|---|---|
| Док-правки safety-логики/распределения/протокола (включая `include/`) | `safety-reviewer` | после каждого значимого изменения |
| Pinmap + силовая/изоляционная схема | `schematic-engineer` + `safety-reviewer` | до шага 3 финализации и до сборки/прошивки |
| BLK-001 blower (accepted design, no per-circuit cutoff) + SR-B6 board-off side effects | `schematic-engineer` + `safety-reviewer` | до схемного подтверждения цепи вентилятора/автомата/SR-B6 поведения |
| BLK-002 общий контактор (топология/feedback/weld/остаточный риск) | `schematic-engineer` + `safety-reviewer` | до утверждения силовой схемы |
| BLK-003 exhaust temp accepted risk | `safety-reviewer` ACK | до финализации FSM/safety spec |
| BLK-005 flame sensor (ADC+photoresistor / opto fallback) | `schematic-engineer` + `safety-reviewer` | до утверждения схемы и до прошивки логики пламени |
| BLK-006 protocol migration | `safety-reviewer` | до любого safety UART кода в прошивке |
| BLK-012 KC868-A6 board verification (ревизия/схема/GPIO/реле/ADC/RS485/питание) | `schematic-engineer` + `safety-reviewer` | до финализации pinmap A6 и до сборки/прошивки |
| Стендовые тесты (A6 board verification §7, noise/ignition для BLK-005, SR-B6 ramp/weld/feedback) | `safety-reviewer` evidence review | до опытной эксплуатации |

> **KC868-A6 (не v3):** официальная схема A6 **доступна** (`KC868-A6-schematic.pdf`), но требует сверки с фактической ревизией платы. Это **не снимает** gates: board-level факты (контакты/драйвер реле, reset/brownout, опто-входы, A3/A4, RS485 без изоляции, питание) подтверждаются **схемой + измерением/даташитами** (`schematic-engineer`), а safety-функции строятся **только внешним слоем Nano**. PCF8574/I2C — **не fail-safe path**. Pinmap-статус «утверждён» по-прежнему требует board test + approvals.

> **KC868-A6v3:** схема **недоступна** (архив/сравнение; v3-артефакты к A6 не применять).

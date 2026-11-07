# Блокеры и проблемы архитектуры (текущее состояние)

Статус: **ACTIVE** — часть блокеров уточнена решениями пользователя (2026-11-08), но схемные/safety-валидационные условия остаются.  
Дата обновления: 2026-11-08  
Источник: safety-review `safety-reviewer` (session `ses_e457842baffeXg77w4V8gW6k71`, вердикт **APPROVE_WITH_CONDITIONS**, docs-only) + решения пользователя по BLK-001…007.  
Область: `docs/architecture/*`, `docs/hardware/*`, `docs/protocol/uart.md`, `include/common_protocol.h`, будущие pinmap/schematic, `src_esp32/`, `src_avr8/`.

> **Правило:** до закрытия CRITICAL/HIGH условий запрещены: финализация pinmap/schematic как «утверждено», реализация safety-протокола в прошивке без `safety-reviewer` APPROVE, ослабление Nano cutoffs/gates/block-stop. Любая реализация — только с `safety-reviewer` (и `schematic-engineer` для силовой части/изоляции/220В). **Номиналы средств защиты/автоматов/контакторов выбираются на отдельном шаге** (не закрывает safety-анализ топологии/feedback/fail-safe).

## Сводка

| ID | Severity | Статус | Тема | Блокирует |
|---|---|---|---|---|
| BLK-001 | CRITICAL | MITIGATED (SCHEMATIC/VALIDATION OPEN) | `ACT_FAN_BLOWER` управляется KC868 relay; Nano может обесточить **всю плату KC868** (сброс реле) + защитный автомат. Не независимый per-circuit blower cutoff. | step 3 schematic, SR-B6/whole-board power-off анализ, режим продувки/аварии, `safety-reviewer` APPROVE |
| BLK-002 | CRITICAL | DESIGN GATE (не «закрыт выбором компонента») | Общий Nano-контактор (ТЭН/подкачка/розжиг): сварка/залипание/несостоявшееся размыкание, single-point failure, feedback/fail-safe outputs. Выбор конкретного устройства — отдельно, но safety-требования обязательны. | schematic, SR-B4/SR-B18 доказательство, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-003 | HIGH | ACCEPTED RISK (DOC DECISION, REQUIRES ACK) | `SEN_THERMOCOUPLE_K`/температура выхлопа — зона обратной связи процесса горения Main Unit, **не зона контроля Nano Safety**. Nano не имеет независимого триггера перегрева выхлопа. | FSM/safety spec (явное принятие риска), `safety-reviewer` ACK; при изменении требований — пересмотр |
| BLK-004 | HIGH | PARTIALLY SPECIFIED (SCHEMATIC OPEN) | Per-actuator Nano hardware backup уточнён пользователем (KC868 board power-off Nano, общий контактор, физическое дублирование клапанов, цирк. насос). Нужна схемная фиксация и запрет обхода. | schematic, actuator wiring, code review |
| BLK-005 | HIGH | PROPOSED / NOT VALIDATED (DEVIATION RISK) | `SEN_FLAME` = фоторезистор на **аналоговый вход KC868**, экранированный кабель + тесты наводок при розжиге; при недостаточности — опторазвязка. Отклонение от SR-B13/S-47 «direct ESP32-S3 GPIO + внешний изолированный формирователь digital/Schmitt; запрещено ADC-raw/PCF8574/opto-I2C path». | schematic, noise/ignition tests, протокол flame/ignition supervision, `safety-reviewer` + `schematic-engineer` APPROVE |
| BLK-006 | HIGH | IN PROGRESS (DOC DRAFT, SAFETY-REVIEW REQUIRED) | Миграция `include/common_protocol.h`/`docs/protocol/uart.md` начата (safety-oriented draft). Требует `safety-reviewer` APPROVE до реализации в прошивке. | любой safety UART код, heartbeat/permit, совместимость |
| BLK-007 | MEDIUM | POLICY SPECIFIED (SCHEMATIC OPEN) | `ACT_PUMP_BOILER`: KC868 relay + возможное дублированное включение Nano (автомат). Оставание включённым в аварии — не проблема по решению пользователя. Нужна схемная фиксация/защита. | pinmap/schematic, управление насосом котла |
| BLK-008 | MEDIUM | OPEN | SR-B6 supervised power-cycle KC868 (whole-board power-off используется как mitigation в BLK-001/004): BOR/Hi-Z/feedback/weld/ramp tests не трассированы. | schematic, Nano power-control, стендовые тесты |
| BLK-009 | MEDIUM | OPEN | Grounding/isolation §7.18 и 220В монтаж/транзиенты (S-48) не выбраны/не подтверждены. | schematic, PCB/wiring, UART/feedback isolation, EMC/safety tests |
| BLK-010 | MEDIUM | OPEN | RS485 S-49 не финализирован. | RS485 design, slave specs |
| BLK-011 | LOW | OPEN | `docs/safety/checklist.md` дополнять по мере фиксации шага 3. | будущие safety-review |

## Детали блокеров и решений

### BLK-001 (CRITICAL → MITIGATED, SCHEMATIC/VALIDATION OPEN) — Вентилятор наддува
**Решение пользователя:** `ACT_FAN_BLOWER` управляется через реле на плате KC868. При необходимости Nano может **обесточить всю плату KC868** и тем самым отключить реле; плюс защитный автомат.  
**Интерпретация:** независимого Nano cutoff **именно в силовой цепи вентилятора** нет; mitigation — coarse action: снятие питания KC868 платой Nano (SR-B6-подобное действие).  
**Остаточные риски/условия:**
- [ ] Обесточивание всей платы KC868 останавливает Main Unit/FSM/heartbeat/прочие реле/логику — это **аварийный** режим, не штатный stop вентилятора. Требуется режимный анализ: продувка/постпродувка vs аварийная остановка, что безопаснее в каждом сценарии.
- [ ] Схема SR-B6: Nano **никогда** не режет своё питание; управляет только питанием KC868; supervised/last-resort, current-limited, no fast cycling, min off-time/limits, 3-state feedback, latch, Nano fail-safe outputs (SR-B14/B15/SR-B12), weld/stuck handling.
- [ ] При пропадании питания KC868 реле на плате должны перейти в безопасное состояние для вентилятора (подтвердить схемой/типом реле/PCF8574 поведением при потере питания).
- [ ] `SEN_AIR_PRESSURE` на Nano остаётся обязательной независимой защитой: без давления/при отказе наддува топливо не подаётся, safety-gates закрываются.
- [ ] Защитный автомат/защита цепи вентилятора 220В — номиналы на отдельном шаге; топология/селективность/монтаж — `schematic-engineer`.
- [ ] `safety-reviewer` APPROVE после схемы/анализа режимов.

### BLK-002 (CRITICAL → DESIGN GATE) — Общий Nano-контактор (ТЭН/подкачка/розжиг)
**Решение пользователя:** не блокер как архитектурный запрет, а вопрос выбора конечного устройства, обеспечивающего коммутацию. Номиналы — отдельный шаг.  
**Safety-уточнение (сохраняется):** выбор устройства **не снимает** требования: fail-safe размыкание (SR-B14/B15/SR-B12/S-42), SR-B4 (ТЭН, cutoff в силовой цепи последовательно с ТЭН; SSR не аварийное отключение), SR-B18 (подкачка, hardware block/stop fail-safe OFF), отсутствие шунтирования Main Unit/KC868/SSR, inrush/AC-категория/ЭМС розжига, детект несостоявшейся отсечки (feedback, **fail-safe полярность**: обрыв/отказ = тревога, не «отсечено»), latch, запрет повторной подачи, оценка сварки/forced-guided/остаточного риска. При сварке safety-gates + независимые пути должны предотвращать опасную подачу топлива/воздуха и неконтролируемый нагрев (с учётом термозащиты ТЭН).  
**Требуется закрыть:** схемой + `schematic-engineer` + `safety-reviewer` APPROVE.

### BLK-003 (HIGH → ACCEPTED RISK, REQUIRES ACK) — Температура выхлопа
**Решение пользователя:** температура выхлопа (`SEN_THERMOCOUPLE_K`/MAX6675, если используется) — **не зона контроля модуля безопасности Nano**; основная задача — обратная связь по процессу горения (Main Unit).  
**Интерпретация:** Nano **не обязан** иметь независимый hardware-триггер по перегреву выхлопа. Защита от опасных режимов опирается на Nano safety-gates, `SEN_AIR_PRESSURE`, прямые Nano датчики (`SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_*`), heartbeat/FSM supervision, общий контактор (ТЭН/подкачка/розжиг), latch/fail-safe.  
**Остаточные риски/условия:**
- [ ] Явно зафиксировать в FSM/safety spec: перегрев выхлопа не является независимым Nano safety interlock.
- [ ] Main Unit использует температуру выхлопа как технологическую обратную связь; при недостоверности/timeout датчика Main Unit должен переходить в безопасный прикладный режим (не продолжать опасный розжиг/работу без валидной обратной связи, где это требуется FSM).
- [ ] Требуется `safety-reviewer` ACK принятого риска/границы ответственности. При изменении требований к защите от перегрева выхлопа — обязательный пересмотр.
- [ ] Placement/интерфейс MAX6675 на Main Unit — по схеме (шум, SPI, диагностика fault), но без требования Nano safety-path.

### BLK-004 (HIGH → PARTIALLY SPECIFIED, SCHEMATIC OPEN) — Per-actuator Nano hardware backup
**Решение пользователя (уточнение):**
- Подача воздуха — управление через PCF8574 relay; Nano может обесточить всю плату KC868 (и реле) — coarse mitigation (см. BLK-001/SR-B6). Плюс Nano safety-gate `ACT_AIR_VALVE_SAFETY` (физическое дублирование/последовательность клапанов) остаётся обязательным инвариантом.
- Нагрев масла, насос подкачки, розжиг — отключаются через Nano-контактор.
- Клапаны дублированы физически (`*_VALVE_MAIN` + `*_VALVE_SAFETY` NC последовательно).
- Циркуляционный насос котла (`ACT_PUMP_BOILER`) — дублированное управление с Nano (см. BLK-007).

| Актуатор/группа | Прикладное управление | Независимый Nano hardware path / mitigation | Fail-safe состояние | Статус |
|---|---|---|---|---|
| `ACT_OIL_VALVE_MAIN` | KC868/PCF8574 relay | **`ACT_OIL_VALVE_SAFETY` NC, только Nano**, последовательно (физ. дублирование) | Safety-gate закрыт | Инвариант |
| `ACT_AIR_VALVE_MAIN` | KC868/PCF8574 relay | **`ACT_AIR_VALVE_SAFETY` NC, только Nano**, последовательно + Nano может обесточить плату KC868 (coarse, SR-B6) | Safety-gate закрыт; board-off аварийно | Инвариант + BLK-001/SR-B6 |
| `ACT_HEATER_OIL` | KC868/PCF8574 relay / SSR pilot | **Общий Nano-контактор** (SR-B4) + термозащита ТЭН/предохранители | Обесточен/отключён | BLK-002 gate |
| `ACT_PUMP_OIL_TRANSFER` | KC868/PCF8574 relay | **Общий Nano-контактор** (SR-B18) | OFF/обесточен | BLK-002 gate |
| `ACT_IGNITION_UNIT` | KC868/PCF8574 relay | **Общий Nano-контактор** | Обесточен | BLK-002 gate |
| `ACT_FAN_BLOWER` | KC868/PCF8574 relay | Nano может обесточить **всю плату KC868** (coarse) + автомат; нет per-circuit Nano cutoff | Аварийно — board-off/автомат; штатно — по FSM | BLK-001/SR-B6 |
| `ACT_PUMP_BOILER` | KC868/PCF8574 relay | Дублированное включение Nano напрямую возможно (схема); автомат. Оставание включённым в аварии — не проблема (решение пользователя) | По схеме; не safety-critical stop | BLK-007 |
| `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_*` | RS485 slave | Slave watchdog/fail-safe defaults (S-49) | Безопасная позиция / OFF | BLK-010 |

### BLK-005 (HIGH → PROPOSED/NOT VALIDATED, DEVIATION RISK) — `SEN_FLAME`
**Решение пользователя:** фоторезистор, заводится на **аналоговый вход KC868**, экранированным кабелем + тесты на практике достаточна ли защита от наводок при розжиге. Если недостаточно — реализация с опторазвязкой.  
**Safety-уточнение:** это **отклонение** от ранее зафиксированного условия SR-B13/S-47 («только direct ESP32-S3 GPIO + внешний гальванически изолированный формирователь digital/Schmitt, fail-safe полярность/диагностика; запрещено KC868 opto/PCF8574/I2C/ADC-raw»). Аналоговый путь через KC868/ADC более чувствителен к наводкам ВВ розжига, дрейфу, делителю/питанию, ошибкам ПО/порога. **Не утверждать без валидации.**  
**Требуется закрыть/валидировать:**
- [ ] Схема входа: источник питания/делитель/фильтр/защита, экранирование, routing от ВВ розжига/220В; поведение при обрыве/КЗ/потере питания датчика = **«нет пламени»** (fail-safe).
- [ ] Алгоритм: пороги с гистерезисом, debounce/интегрирование, явная диагностика `invalid/saturated/stuck`, таймауты розжига/потери пламени, latch/no auto-restart.
- [ ] Практические тесты ЭМС/наводок **во время розжига** и при всех режимах вентилятора/контакторов; критерии PASS/FAIL зафиксировать.
- [ ] Если наводки/нестабильность — **обязательный переход** на опторазвязку/внешний изолированный формирователь (digital/Schmitt) к **direct ESP32-S3 GPIO** (предпочтительный fail-safe вариант).
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

## Обязательные approval gates

| Gate | Кто | Когда |
|---|---|---|
| Док-правки safety-логики/распределения/протокола (включая `include/`) | `safety-reviewer` | после каждого значимого изменения |
| Pinmap + силовая/изоляционная схема | `schematic-engineer` + `safety-reviewer` | до шага 3 финализации и до сборки/прошивки |
| BLK-001 blower whole-board power-off режимы + SR-B6 | `schematic-engineer` + `safety-reviewer` | до реализации аварийных режимов/схемы питания KC868 |
| BLK-002 общий контактор (топология/feedback/weld/остаточный риск) | `schematic-engineer` + `safety-reviewer` | до утверждения силовой схемы |
| BLK-003 exhaust temp accepted risk | `safety-reviewer` ACK | до финализации FSM/safety spec |
| BLK-005 flame sensor (ADC+photoresistor / opto fallback) | `schematic-engineer` + `safety-reviewer` | до утверждения схемы и до прошивки логики пламени |
| BLK-006 protocol migration | `safety-reviewer` | до любого safety UART кода в прошивке |
| Стендовые тесты (KC868 analysis §10, noise/ignition для BLK-005, SR-B6 ramp/weld/feedback) | `safety-reviewer` evidence review | до опытной эксплуатации |

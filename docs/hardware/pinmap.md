# Пин-мап (DRAFT / PLANNING PROPOSAL — NOT APPROVED)

> **СТАТУС: ЧЕРНОВИК ПЛАНИРОВАНИЯ. НЕ УТВЕРЖДЕНО. ФИНАЛИЗАЦИЯ ШАГА 3 ЗАБЛОКИРОВАНА.**
>
> Этот документ — **предварительная (draft) проработка pinmap** для шага 3. Он **не фиксирует** окончательные назначения пинов/ресурсов и **не может** использоваться как основание для «утверждённой» схемы, разводки, прошивки safety-логики или монтажа.
>
> **Блокировка финализации:** до закрытия CRITICAL/HIGH условий в `docs/architecture/blockers.md` (и выполнения approval gates) финализация pinmap/schematic как «утверждено» **запрещена** (см. blockers.md, checklist, `docs/hardware/kc868-a6_analysis.md` §6/§7).
>
> ⚠️ **Main Unit = KC868-A6 (не v3).** Раздел §2 приведён к **A6 (ESP32-WROOM-32)**. Пин-мап A6 остаётся **DRAFT/TBD** до board test и approvals. **v3-данные (ESP32-S3) — только в `docs/hardware/kc868-a6v3_analysis.md`; к A6 не применять.**
>
> **Обязательные ревью перед любым утверждением/реализацией:** `schematic-engineer` + `safety-reviewer` (включая силовую часть 220В, изоляцию/grounding §7.18, fail-safe уровни, протокол BLK-006, BLK-002/005 gates).

## Контекст / инварианты (сохранять без ослабления)

- **Safety Unit:** Arduino Nano (ATmega328P), `env:arduino_safety`, `src_avr8/` — **независимый, не заменяется**. Nano имеет **финальное право блокировки**.
- **Main Unit (safety-gated):** KC868-A6 (не v3) / ESP32-WROOM-32, `env:esp32_core` (`board = esp32dev`; legacy env подходит). Условное принятие — `docs/hardware/kc868-a6_analysis.md` §6. **Замена v3; v3-данные в §2 — справочные, не для A6.**
- **Nano safety-gates:** `ACT_OIL_VALVE_SAFETY`, `ACT_AIR_VALVE_SAFETY` (NC) — **только Nano**, независимые контуры, Main Unit обхода не имеет.
- **Прямые датчики Nano (инварианты):** `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_MIN`, `SEN_FLOAT_MAX` — **только прямые входы Nano**. Не переносить на KC868 опто/PCF8574/I2C, не на FPC/RS485.
- **KC868 PCF8574/I2C (реле `0x24`, опто `0x22`, общая шина с RTC/OLED/DAC): не независимый/fail-safe safety path.** Latch/reset/brownout/3.3V sag/`I2C_FAULT` возможны. Onboard реле — **только прикладное управление** при Nano hardware safety-gate/cutoff/block-stop; опто/ADC — технологические/не safety-критичные, кроме `SEN_FLAME` (BLK-005, PROPOSED/NOT VALIDATED, не утверждено).
- **Safety link:** dedicated point-to-point UART KC868↔Nano (**не RS485**). USB-C + CH340C — debug (не native USB). Nano D0/D1 конфликтуют с USB программированием/отладкой — обязательна процедура/переключение/альтернатива.
- **BLK-001 (ACCEPTED DESIGN, schematic open):** `ACT_FAN_BLOWER` — KC868 relay; **per-circuit Nano cutoff не требуется**. Nano может аварийно **обесточить всю плату KC868** (SR-B6, coarse) + защитный автомат. Топливная безопасность — safety-gates + `SEN_AIR_PRESSURE` + Nano cutoff ТЭН/подкачки/розжига. Board-off убивает Main Unit/FSM — **не штатная управляемая постпродувка**.
- **BLK-002 (DESIGN GATE):** общий Nano-контактор/силовое реле (**не самозамыкающееся, без самоподхвата, управляется только Nano, без шунтирования**) для `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT`. `ACT_FAN_BLOWER` в обход. Для ТЭН — SSR/драйвер (не аварийное отключение) + cutoff последовательно с ТЭН + **термовыключатель/«теплушка» на корпусе нагреваемого бака** + термопредохранитель/предохранители (SR-B4). Draft принятия остаточного риска залипания — **ACK required**.
- **BLK-003 (ACCEPTED RISK, ACK requested):** `SEN_THERMOCOUPLE_K`/MAX6675 — технологическая обратная связь Main Unit, **не зона Nano Safety**.
- **BLK-005 (PROPOSED/NOT VALIDATED):** `SEN_FLAME` — фоторезистор на **аналоговый вход KC868** + экранирование + PASS/FAIL ЭМС-тесты при розжиге; fallback — опторазвязка/изолированный формирователь (digital/Schmitt) на **direct ESP32 GPIO**. **Отклонение от SR-B13/S-47. До `safety-reviewer`+`schematic-engineer` APPROVE не safety-утверждено и не реализуется.**
- **BLK-006 (DRAFT, firmware blocked):** `include/common_protocol.h` / `docs/protocol/uart.md` — safety draft; реализация в прошивке только после `safety-reviewer` APPROVE.
- **BLK-007 (POLICY SPECIFIED, schematic open):** `ACT_PUMP_BOILER` — KC868 relay; возможно дублированное включение Nano по схеме. Оставание ON в аварии допустимо по решению пользователя; схема pending.
- **SR-B6:** supervised power-cycle 12В KC868 — только Nano, last resort; Nano **не режет своё питание**.

## Источники
- `rnd/peripherals.md`, `docs/architecture/{modules,blockers}.md`, `docs/hardware/{periph_distribution,io_analysis,kc868-a6_analysis,kc868-a6v3_analysis}.md`
- `docs/hardware/kc868-a6_analysis.md` — **A6**: ESP32-WROOM-32, CH340C, PCF8574 `0x22/0x24`, 4×ADC, 2×1-Wire, RS485 (14/27), RS232 (16/17), **официальная схема доступна**
- `rnd/KC868-A6v3 ESP32-S3 IO pins define.txt` / `rnd/KC868-A6v3 ESPHome.txt` — **v3 (ESP32-S3)**; к A6 **не применять**, только архив/сравнение
- `docs/protocol/{uart,rs485}.md`, `include/common_protocol.h` (BLK-006 draft)
- `docs/safety/{checklist,fsm_safety_boundary,blk002_closure_package}.md`, даташиты

## Легенда статусов (draft)
| Статус | Значение |
|---|---|
| **FIXED-DRAFT** | Роль/инвариант/класс зафиксирован архитектурно; физический пин/клемма/контакт/номинал **не утверждён**. |
| **CANDIDATE** | Предлагаемый вариант; требует подтверждения схемой/strapping/уровнями/ревью. |
| **TBD** | Уточнение обязательно. **Не финальное.** |
| **BLOCKED** | Нельзя финализировать/утверждать/реализовывать до gate/блокера/approval. |

## Обязательные условия до финализации (НЕ ВЫПОЛНЕНЫ)
- [x] **A6 official data разобран**: ESP32-WROOM-32; PCF8574 `0x22` inputs / `0x24` relays; ADC A1=36/A2=39/A3=34/A4=35; 1-Wire 32/33; I2C SDA4/SCL15; RS485 14/27; RS232 16/17; SPI 5/18/19/23; DAC 25/26; официальная схема A6 доступна. См. `docs/hardware/kc868-a6_analysis.md`.
- [x] **v3-артефакты помечены как архив** (`rnd/KC868-A6v3 *`) — к A6 не применять.
- [ ] **A6 board-level verification** (`docs/hardware/kc868-a6_analysis.md` §7): сверить ревизию со схемой; MCU/flash/PSRAM; поведение PCF8574/реле при reset/brownout/power loss; тип/группировка контактов (SPDT), драйвер ULN2003A; параметры/изоляция опто-входов EL357N; свободные GPIO/strapping; ADC A1–A4 (A3/A4 0–5 В vs 4–20 мА); RS485 без изоляции.
- [ ] Grounding/isolation §7.18: архитектура A/B до схемы; **не смешивать барьеры**. Уровни/изоляция safety UART и supervised `KC868_12V` feedback.
- [ ] BLK-006: `safety-reviewer` APPROVE протокола до прошивки.
- [ ] BLK-005: `SEN_FLAME` схема+алгоритм+PASS/FAIL ЭМС-тесты+fallback; APPROVE до реализации (переназначить на подтверждённые ADC A6).
- [ ] BLK-002: схема Nano cutoff (no self-latch/no bypass), SR-B4 цепь ТЭН + теплушка (тип сброса ACK), SR-B18, feedback fail-safe, FMEA/residual weld ACK, evidence.
- [ ] SR-B14/SR-B15: уровни Nano outputs при reset/brownout/Hi-Z/WDT/bootloader, pull-down/pull-up, отпускание катушек.
- [ ] SR-B6/BLK-008: ключ 12В, soft-start/current limit, 3-state feedback, no fast cycling, weld/stuck/latch.
- [ ] S-48/BLK-009: 220В монтаж/изоляция/УЗО/предохранители/транзиенты.
- [ ] S-49/BLK-010: RS485 сегменты, `DE/RE`/timeouts, slave fail-safe defaults.
- [ ] BLK-003: `safety-reviewer` ACK `docs/safety/fsm_safety_boundary.md`.
- [ ] BLK-007: схема `ACT_PUMP_BOILER` (Nano enable/приоритеты/автомат/обратное питание/изоляция).

## 1. Safety Unit — Arduino Nano (DRAFT, NOT APPROVED)

**Платформа:** Arduino Nano (ATmega328P, 5V), `env:arduino_safety`.  
**Приоритет:** прямые safety sensors → safety-gates → общий cutoff driver/feedback → SR-B6 control/feedback → safety UART → диагностика/резерв.  
**Ограничения:** D0/D1 конфликт с USB; A6/A7 — только ADC; Hi-Z при reset/bootloader/brownout — риск (SR-B14/SR-B15). **Назначения ниже CANDIDATE/TBD — не финал.**

| Nano pin | DRAFT роль / сигнал | Периферия/функция (`rnd/peripherals.md`/SR) | Тип/уровень/развязка (TBD схемой) | Fail-safe требование | Статус | Примечание |
|---|---|---|---|---|---|---|
| D0 (RX) | Safety UART RX (← KC868) | Safety link (BLK-006) | UART RX, уровни/изоляция §7.18 | Потеря/invalid => safe state/latch | **CANDIDATE** | Конфликт с USB Nano. Процедура прошивки: отключение/переключатель линии или обесточивание/удержание KC868 в reset; альтернативы — только с `safety-reviewer`. |
| D1 (TX) | Safety UART TX (→ KC868) | Safety link | UART TX | Idle/Hi-Z не создаёт ложный permit | **CANDIDATE** | См. D0. |
| D2 | `SEN_AIR_PRESSURE` input | `SEN_AIR_PRESSURE` — **прямой Nano, инвариант** | Digital threshold, развязка/фильтр/подтяжка TBD | Нет/недостоверно давление => запрет опасных действий/топлива по Nano-политике | **CANDIDATE** | Пороговый (вкл/выкл). Подтвердить выход датчика, полярность, дребезг, ЭМС, обрыв/КЗ поведение. |
| D3 | `SEN_FLOAT_MIN` input | `SEN_FLOAT_MIN` — **прямой Nano, инвариант** | Геркон, подтяжка/фильтр TBD | Критический минимум/неопределённость => блокировка | **CANDIDATE** | Подтвердить NC/NO, fail-safe уровень. |
| D4 | `SEN_FLOAT_MAX` input | `SEN_FLOAT_MAX` — **прямой Nano, инвариант** | Геркон | Max/неопределённость => block/stop подкачки | **CANDIDATE** | SR-B18 контекст. |
| D5 | `ACT_OIL_VALVE_SAFETY` driver | `ACT_OIL_VALVE_SAFETY` — **только Nano**, NC safety-gate | Output→драйвер→независимая цепь; NO relay energized-to-open / гидравлика NC TBD схемой | Авария/timeout/latch/reset/brownout/Hi-Z/WDT => **закрыт** | **CANDIDATE** | Инвариант. No bypass by Main Unit. 220В изоляция/транзиенты S-48. |
| D6 | `ACT_AIR_VALVE_SAFETY` driver | `ACT_AIR_VALVE_SAFETY` — **только Nano** | Аналогично D5 | **Закрыт** при unsafe | **CANDIDATE** | Инвариант. |
| D7 | `ACT_GROUP_CUTOFF` driver | Общий cutoff `ACT_HEATER_OIL`+`ACT_PUMP_OIL_TRANSFER`+`ACT_IGNITION_UNIT` (BLK-002) | Output→драйвер катушки/управления силового реле; de-energize-to-trip TBD | Reset/brownout/Hi-Z/WDT/потеря питания Nano => **разомкнут/обесточен** | **CANDIDATE (BLOCKED BLK-002)** | **Не самозамыкающийся, без самоподхвата/шунтирования, только Nano.** `ACT_FAN_BLOWER` в обход. SR-B4/SR-B18 цели — только подтверждённой схемой. |
| D8 | `ACT_GROUP_CUTOFF_FB` | Feedback общего cutoff (BLK-002) | Вход feedback, тип TBD | Обрыв/КЗ/неизвестность = **`UNKNOWN/FAULT`, не `OFF_CONFIRMED`** | **CANDIDATE (BLOCKED BLK-002)** | Aux/mirror/voltage sense — схема; не шунтирует силовую цепь. `contactorFb` BLK-006 draft. |
| D9 | `KC868_12V_CTRL` | SR-B6 supervised power-cycle 12В KC868 (BLK-001/004 mitigation) | Output→силовой ключ 12В (NO energized-to-permit) + soft-start/current limit TBD | Nano reset/brownout/loss/WDT => отпускание по схеме; Nano self-power не трогается | **CANDIDATE (BLOCKED SR-B6/BLK-008)** | Last resort. No fast cycling, min off-time/limits, latch. |
| D10 | `KC868_12V_FB` | 3-state supervised feedback питания KC868 | Вход `present/absent/fault-unknown` (EOL/4–20mA/2-channel) TBD | Обрыв/отказ = `fault/unknown` (тревога), не подтверждение отсечки | **CANDIDATE (BLOCKED SR-B6/§7.18)** | Изоляция при раздельных 0В. `kc868PowerFb` BLK-006 draft. |
| D11 | `SEN_OIL_TEMP_SAFETY` 1-Wire | `SEN_OIL_TEMP_SAFETY` — **прямой Nano, инвариант** (DS18B20) | 1-Wire + подтяжка/защита/экранирование TBD | Timeout/fault/invalid => консервативная защита | **CANDIDATE** | Отдельная линия/защита от технологических DS18B20 Main Unit. |
| D12 | Резерв/сервис | Non-safety service (TBD) | TBD | Hi-Z не опасен | **TBD** | Не расходовать без пересчёта бюджета. |
| D13 | Резерв/индикация | Non-safety | TBD | Non-safety | **TBD** | — |
| A0 | `ACT_PUMP_BOILER` Nano enable (опция) | BLK-007 дублированное включение Nano (если схемой подтверждено) | Output→драйвер/цепь, обратное питание/изоляция TBD | Hi-Z/de-energized = не включает; не обходит автоматику/изоляцию | **TBD (BLK-007 schematic)** | Только по схеме. Иначе резерв. |
| A1–A5 | Резерв/диагностика | — | Digital/ADC TBD | Non-safety | **TBD** | A4/A5 — I2C pins ATmega328P (не планируется). |
| A6, A7 | Резерв (ADC only) | — | **Analog input only** | — | **TBD** | **Не digital I/O.** |

### Nano I/O budget (DRAFT)
| Категория | Кол-во | Пины (кандидат) | Статус |
|---|---:|---|---|
| Прямые safety sensors | 4 | D2,D3,D4,D11 | FIXED-DRAFT инварианты |
| Safety-gate outputs | 2 | D5,D6 | FIXED-DRAFT |
| Общий cutoff driver+feedback | 2 | D7,D8 | CANDIDATE, **BLK-002** |
| SR-B6 control+feedback | 2 | D9,D10 | CANDIDATE, **SR-B6/BLK-008** |
| Safety UART | 2 | D0,D1 | CANDIDATE, D0/D1 conflict |
| BLK-007 опция `ACT_PUMP_BOILER` Nano enable | 0–1 | A0? | TBD |
| Резерв (D12,D13,A1–A5,A6–A7) | 9 | — | часть ADC-only; бюджет tight при расширении |
| **Минимум (без BLK-007)** | **12** | D0–D11 | Подтверждает `io_analysis.md` |

**Митигации дефицита:** MAX6675 — на Main Unit (BLK-003, ACK req.), не на Nano; внешний watchdog — схемно; любой доп. контакт/диагностика — пересчёт + `safety-reviewer`.

## 2. Main Unit — KC868-A6 / ESP32-WROOM-32 (DRAFT, NOT APPROVED)

> ⚠️ **A6: официальной схемы-исключения нет — схема A6 доступна.** Таблица ниже — подтверждённые ресурсы A6; назначение (роль) остаётся **DRAFT**, физические пины — после board test (`kc868-a6_analysis.md` §7) и approvals.

**Платформа (A6):** KC868-A6 (не v3), **ревизия v1.3** (подтверждено 2026-11-09), ESP32-WROOM-32, USB-C + CH340C, DIN/12В (условное принятие Main Unit, `kc868-a6_analysis.md` §6). `env:esp32_core`, `board = esp32dev`.  
**Safety constraint:** onboard реле/опто-входы через PCF8574/I2C — **не fail-safe safety path**. Реле — **только прикладное управление** при Nano hardware safety-gate/cutoff/block-stop. Опто-входы — технологические/не safety-критичные. `SEN_FLAME` (BLK-005) — **PROPOSED/NOT VALIDATED** на аналоговом входе KC868; до APPROVE не safety-утверждено.  
**GPIO note (A6):** свободные GPIO **официальным списком не подтверждены**; кандидаты: 12/13 (extend serial), 25/26 (DAC), 16/17 (RS232), 32/33 (1-Wire), 21/2/22 (LoRa/nRF) — все **CANDIDATE**, требуют board test и отказа от соответствующей функции. GPIO34–39 input-only; GPIO0/2/5/12/15 — strapping. **Схема A6 доступна**, но требует сверки с ревизией платы; поведение PCF8574/реле при reset/brownout/power loss — только board test.

> 📋 Предварительный план «какая периферия к какой линии A6» с бюджетом свободных линий/дефицитом: `docs/hardware/kc868-a6_connection_plan.md` (**DRAFT / NOT APPROVED**, не заменяет этот pinmap).

### 2.1. Подтверждённые/занятые ресурсы платы A6 (board mapping, DRAFT reference)

| Ресурс/интерфейс | Подтверждённое назначение (источник) | DRAFT использование в проекте | Safety/ограничения | Статус |
|---|---|---|---|---|
| USB-C + CH340C | USB-serial (не native USB) | Отладка/прошивка/лог Main Unit | **Не safety link.** | **FIXED-DRAFT** (интерфейс) |
| RS485 (GPIO14 RXD, GPIO27 TXD; MAX13487EESA) | Встроенный RS485, **без гальваноизоляции** (схема A6) | Non-safety RS485 master: `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` (S-49) | **Не safety.** DMX512/Modbus — раздельные сегменты (S-49/BLK-010). Терминирование/TVS — схема. | **CANDIDATE** |
| RS232 (GPIO16 RXD, GPIO17 TXD; SP3232EEN) | Встроенный RS232 | Резерв/сервис/HMI (non-safety) | Не safety. Взаимоисключения/использование — pinmap. | **TBD** |
| I2C (GPIO4 SDA, GPIO15 SCL) | Общая шина: PCF8574 `0x24` outputs, PCF8574 `0x22` inputs | Onboard реле/опто; `I2C_FAULT` отражать в heartbeat | **Единый домен отказа, не fail-safe.** | **FIXED-DRAFT** (шина занята) |
| 1-Wire 1 (GPIO32) | 1-Wire (official pin define; pull-up on PCB — verify) | Технологические DS18B20 (non-safety) | Не safety; `SEN_OIL_TEMP_SAFETY` **не здесь** (прямой Nano). | **CANDIDATE** |
| 1-Wire 2 (GPIO33) | 1-Wire (official pin define) | Технологические DS18B20, шина 2 | Не safety. | **CANDIDATE** |
| ADC A1 (GPIO36) | Схема A6: делитель **0–5 В** | **Кандидат под `SEN_FLAME` (BLK-005, PROPOSED/NOT VALIDATED)** | **BLK-005:** не safety-утверждено до APPROVE; fail-safe пороги/диагностика/ЭМС тесты обязательны. Input-only. | **CANDIDATE (BLOCKED BLK-005)** |
| ADC A2 (GPIO39) | Схема A6: делитель **0–5 В** | Резерв / технологический / альтернативный `SEN_FLAME` кандидат | Аналогично A1; **voltage-канал** под фоторезистор. Input-only. | **TBD** |
| ADC A3 (GPIO34) | Схема A6: ОУ/BAT54S; режим **0–5 В vs 4–20 мА TBD** | **Не использовать для `SEN_FLAME` voltage path** до подтверждения | Входной фронт/диапазон — измерение. Input-only. | **TBD** |
| ADC A4 (GPIO35) | Схема A6: ОУ/BAT54S; режим **0–5 В vs 4–20 мА TBD** | **Не использовать для `SEN_FLAME` voltage path** до подтверждения | Аналогично A3. Input-only. | **TBD** |
| SPI socket (GPIO5 CS, GPIO23 MOSI, GPIO19 MISO, GPIO18 SCK) | Сокет LoRa/nRF24 | **Кандидат** для MAX6675/SPI (`SEN_THERMOCOUPLE_K`, BLK-003, non-safety) при свободном сокете | MAX6675 — технологическая обратная связь, не Nano safety. | **CANDIDATE (BLK-003 ACK req.)** |
| Сокет LoRa/nRF (GPIO21 LoRa RST, GPIO2 DIO0, GPIO22 nRF CE) | Заняты сокетом | **Не назначать** при использовании сокета | GPIO2 — strapping. | **FIXED-DRAFT: занято при установленном модуле** |
| DAC (GPIO25 DAC2, GPIO26 DAC1 + LM258DR) | 0–10 В канал (схема A6) | Привод заслонки (non-safety) / резерв | Не safety. | **CANDIDATE** |
| Extend serial (GPIO12 TXD, GPIO13 RXD) | Доп. UART (official pin define) | Кандидат под safety UART к Nano | GPIO12 — strapping; проверить boot. | **CANDIDATE (board test)** |
| PCF8574 `0x22` inputs 0–5 | **6 опто-входов EL357N** (12 В), inverted | **Только технологические/не safety-критичные** дискретные сигналы | Не fail-safe; задержка/polling/I2C fault. | **TBD** (конкретный вход не назначен) |
| PCF8574 `0x24` outputs 0–5 | **6 реле SPDT** (ULN2003A), inverted | Прикладное управление актуаторами **только с Nano hardware backup** | **Не safety path.** Поведение POR/reset/brownout — board test. | **CANDIDATE** (назначение relay# TBD) |

### 2.2. Кандидатные назначения onboard реле KC868 (DRAFT, НЕ ФИНАЛ, НЕ safety path)

| KC868 relay (PCF8574 `0x24`) | DRAFT кандидат нагрузки | Периферия | Nano hardware backup / условие | Fail-safe примечание | Статус |
|---|---|---|---|---|---|
| Relay #1 (ch0) | `ACT_FAN_BLOWER` | Вентилятор наддува | BLK-001 ACCEPTED: Nano может **обесточить всю плату KC868** (SR-B6, coarse) + автомат; **per-circuit cutoff не требуется**. Топливная безопасность — safety-gates+`SEN_AIR_PRESSURE`+общий Nano cutoff ТЭН/подкачка/розжиг. | Board-off = side effect, не штатная продувка. Поведение при потере питания KC868 — **board test** (сверить со схемой A6 ревизии + измерение); не использовать как safety path. | **CANDIDATE (BLK-001/SR-B6 board test)** |
| Relay #2 (ch1) | `ACT_OIL_VALVE_MAIN` | Рабочий клапан топлива NC | **`ACT_OIL_VALVE_SAFETY` NC только Nano** последовательно (инвариант) | При latch/I2C fault/board-off клапан может остаться в последнем состоянии платы, но safety-gate Nano перекрывает подачу. | **CANDIDATE** |
| Relay #3 (ch2) | `ACT_AIR_VALVE_MAIN` | Рабочий клапан воздуха NC | **`ACT_AIR_VALVE_SAFETY` NC только Nano** последовательно; SR-B6 coarse board-off возможен | Аналогично. | **CANDIDATE** |
| Relay #4 (ch3) | `ACT_IGNITION_UNIT` | Блок розжига 220В | В группе **общего Nano cutoff** (BLK-002). KC868 relay — только прикладное. | При latch/залипании реле платы Nano cutoff обесточивает группу (если схема подтверждена). | **CANDIDATE (BLK-002 gate)** |
| Relay #5 (ch4) | `ACT_PUMP_OIL_TRANSFER` (прикладное) или SSR pilot? | Насос подкачки | **SR-B18 + BLK-002:** Nano hardware block/stop fail-safe OFF (общий cutoff). KC868 relay **не** безопасный останов. | Нельзя опираться на KC868 relay для safe stop. | **CANDIDATE (BLK-002/SR-B18)** |
| Relay #6 (ch5) | `ACT_PUMP_BOILER` | Насос котлового контура | BLK-007: KC868 relay + возможное дублированное включение Nano (схема). Оставание ON в аварии допустимо. | Подтвердить обратное питание/изоляцию/автомат/приоритеты. | **CANDIDATE (BLK-007 schematic)** |
| Внешний SSR/драйвер (не onboard relay) | `ACT_HEATER_OIL` pilot control | ТЭН подогрева | **SR-B4/BLK-002:** SSR/драйвер **не** аварийное отключение. Общий Nano cutoff последовательно с ТЭН + теплушка на корпусе бака + термопредохранитель/предохранители. | Управляющая линия SSR — pull-down, Hi-Z=safe; силовая часть по схеме. | **CANDIDATE (direct GPIO TBD, BLK-002)** |

**Примечание:** подтверждены **6 реле SPDT (ULN2003A) через PCF8574 `0x24` (ch0–ch5, inverted)** и **6 опто-входов EL357N через PCF8574 `0x22` (ch0–ch5, inverted)**. Назначение relay#1–#6 (ch0–ch5) остаётся **DRAFT/CANDIDATE**; группировка COM, изоляция, поведение при reset/brownout/power loss — **board test**. 6 onboard реле покрывают прикладные каналы только при вынесении ТЭН на внешний SSR/драйвер.

### 2.3. Кандидаты direct ESP32 GPIO — A6 (DRAFT — VERIFY board test)

> **A6: официального списка свободных GPIO нет.** Кандидаты ниже — только при **отказе от соответствующей штатной функции** и после board test/strapping-проверки. **Не фиксировать конкретные GPIO до board test.** GPIO34–39 — **input-only**. GPIO0/2/5/12/15 — **strapping** (boot); GPIO12/13 — «extend serial», GPIO12 strapping-sensitive.

| Кандидат (verify) | Возможная DRAFT роль | Ограничения | Статус |
|---|---|---|---|
| GPIO13 (extend serial RXD) | Safety UART TX/RX (KC868 ↔ Nano) | Свободен только если не используется extend-serial; уровни/изоляция §7.18; не RS485. | **CANDIDATE (TBD verify)** |
| GPIO12 (extend serial TXD) | Safety UART (альтернатива) | **Strapping** (MTDI) — проверить boot; иначе не использовать. | **CANDIDATE (TBD verify)** |
| GPIO25 / GPIO26 (DAC) | `ACT_HEATER_OIL` SSR pilot / диагностика | Свободны только если DAC-канал не используется; драйвер + pull-down, Hi-Z=safe. | **CANDIDATE (TBD verify)** |
| GPIO16 / GPIO17 (RS232) | Резерв при отказе от RS232 | Конфликт с RS232 (SP3232EEN). | **CANDIDATE (TBD verify)** |
| GPIO32 / GPIO33 (1-Wire) | Резерв при отказе от 1-Wire | Конфликт с 1-Wire. | **CANDIDATE (TBD verify)** |
| GPIO21 / GPIO2 / GPIO22 (LoRa/nRF) | Резерв при отказе от сокета | GPIO2 — strapping; конфликт с сокетом. | **CANDIDATE (TBD verify)** |

### 2.4. Safety UART link (KC868 ↔ Nano) — placeholders (BLK-006 / §7.18)

| Параметр | DRAFT значение / placeholder | Статус | Примечание |
|---|---|---|---|
| Топология | Dedicated point-to-point UART, полнодуплекс (кандидат) | **FIXED-DRAFT** | **Не RS485.** Не использовать общий сегмент RS485 S-49. |
| KC868 side | Direct ESP32 GPIO UART, кандидаты GPIO13/12 (extend serial) | **CANDIDATE (TBD board test)** | USB-C/CH340C не используется для safety. Подтвердить routing/уровни/конфликты/strapping измерением. |
| Nano side | D0 RX / D1 TX (кандидат) | **CANDIDATE** | Конфликт с USB Nano; процедура прошивки обязательна. |
| Уровни | 5V (Nano) ↔ 3.3V (ESP32), двустороннее согласование | **TBD** | ESP32 не 5V tolerant; Nano RX нужен достаточный VIH. Трансляторы уровней ≠ изоляция. |
| Изоляция | По выбранной архитектуре §7.18 (A или B), **не смешивать** | **BLOCKED (§7.18/schematic)** | При раздельных 0В — digital isolator/оптопары; трансляторы требуют общей земли. |
| Скорость | Единая, ≤19200 бод (норматив `uart.md`) | **TBD (после BLK-006 APPROVE/тестов)** | Текущие 115200/9600 — недопустимое расхождение; миграция. |
| Формат/CRC/seq/protoVer | `include/common_protocol.h` safety draft | **BLOCKED (BLK-006, `safety-reviewer` APPROVE)** | CRC algorithm/endianness/packing/test vectors/unknown handling — фиксировать до прошивки. |
| Таймауты | Heartbeat 100–200 мс, timeout ≤3 периодов (≤600 мс) | **FIXED-DRAFT** (норматив) | Обе стороны fail-safe. |
| Разъём/жгут | TBD (экранирование, длина, отвод от 220В/ВВ розжига) | **TBD** | S-48/ЭМС. |

## 3. Пер-периферийная таблица (ВСЕ коды `rnd/peripherals.md`)

> Статусы: **FIXED-DRAFT / CANDIDATE / TBD / BLOCKED**. Физические пины/клеммы/номиналы не утверждены.

### 3.1. Исполнительные механизмы (актуаторы)

| Код (источник) | Назначение | Main Unit (KC868) DRAFT ресурс | Nano DRAFT роль/ресурс | Nano hardware backup / safety note | Статус | Блокеры/условия |
|---|---|---|---|---|---|---|
| `ACT_IGNITION_UNIT` | Блок розжига, 220В | Прикладное: onboard relay кандидат (PCF8574 `0x24`, не safety path) | D7 кандидат — **общий Nano cutoff group** (BLK-002) | Независимое обесточивание Nano; KC868 relay не safety path. ЭМС ВВ розжига — схема/тесты. | **CANDIDATE** | BLK-002, S-48 |
| `ACT_OIL_VALVE_MAIN` | Рабочий клапан топлива NC, 220В | Прикладное: onboard relay кандидат | Не управляет; перекрывается safety-gate | **Только при `ACT_OIL_VALVE_SAFETY` NC Nano последовательно.** | **CANDIDATE** | Safety-gate инвариант, схема |
| `ACT_OIL_VALVE_SAFETY` | Safety-gate клапан топлива NC, 220В | **Не управляет** | D5 кандидат — **только Nano**, независимый контур | Fail-safe NC closed; обход Main Unit невозможен. | **FIXED-DRAFT** (роль/инвариант) | SR-B14/SR-B15, S-48 |
| `ACT_AIR_VALVE_MAIN` | Рабочий клапан воздуха NC, 220В | Прикладное: onboard relay кандидат | Не управляет; перекрывается safety-gate | Последовательно с `ACT_AIR_VALVE_SAFETY`. SR-B6 coarse board-off возможен. | **CANDIDATE** | Safety-gate инвариант |
| `ACT_AIR_VALVE_SAFETY` | Safety-gate клапан воздуха NC, 220В | **Не управляет** | D6 кандидат — **только Nano** | Fail-safe NC closed. | **FIXED-DRAFT** | SR-B14/SR-B15, S-48 |
| `ACT_FAN_BLOWER` | Вентилятор наддува, 220В | Прикладное: onboard relay кандидат | D9/D10 контекст SR-B6: Nano может **обесточить всю плату KC868** (coarse) | **Per-circuit Nano cutoff не требуется (BLK-001 ACCEPTED).** Топливо: safety-gates + `SEN_AIR_PRESSURE` + Nano cutoff ТЭН/подкачка/розжиг. Автомат 220В. | **CANDIDATE** | BLK-001 schematic side effects, SR-B6/BLK-008, S-48 |
| `ACT_HEATER_OIL` | ТЭН подогрева, 220В | ПИД/прикладное: **внешний SSR/драйвер** pilot от direct GPIO кандидат (verify) или иной pilot; **не onboard relay как силовой путь по умолчанию** | D7 кандидат — **общий Nano cutoff** в силовой цепи последовательно с ТЭН (SR-B4/BLK-002) | SSR **не** аварийное отключение. + **термовыключатель/«теплушка» на корпусе нагреваемого бака** + термопредохранитель/предохранители. Тип сброса теплушки — ACK. | **BLOCKED (BLK-002 design gate)** | SR-B4, BLK-002, residual weld ACK, S-48 |
| `ACT_PUMP_OIL_TRANSFER` | Насос подкачки топлива, 220В | Прикладное: onboard relay кандидат (не безопасный останов) | D7 кандидат — **общий Nano cutoff**, hardware block/stop fail-safe OFF (SR-B18/BLK-002) | Не опираться на KC868 relay. | **BLOCKED (BLK-002/SR-B18)** | BLK-002, S-48 |
| `ACT_PUMP_BOILER` | Циркуляционный насос котла, 220В | Прикладное: onboard relay кандидат (PCF8574) | A0 **TBD/опция** — дублированное включение Nano по схеме (BLK-007) | Оставание ON в аварии допустимо по решению пользователя. Подтвердить приоритеты/обратное питание/изоляцию/автомат. Nano stop не обязателен по умолчанию (SR-B17). | **CANDIDATE (BLK-007 schematic open)** | BLK-007, S-48 |
| `ACT_DAMPER_ACTUATOR` | Электропривод шибера (перспектива), 220В через RS485 | RS485 master (GPIO17/18) → slave устройство | **Не управляет** | Non-safety. Slave watchdog/fail-safe safe position. DMX/Modbus сегментация S-49. | **CANDIDATE (RS485, BLK-010 open)** | S-49/BLK-010, slave spec |
| `ACT_PUMP_FLOOR_1` | Насос тёплого пола 1 эт. (перспектива) | RS485 slave | **Не управляет** | Non-safety. Обычно fail-safe OFF + watchdog slave. | **CANDIDATE (RS485)** | S-49/BLK-010 |
| `ACT_PUMP_FLOOR_B` | Насос тёплого пола подвала (перспектива) | RS485 slave | **Не управляет** | Non-safety. | **CANDIDATE (RS485)** | S-49/BLK-010 |

### 3.2. Датчики — безопасность и горение

| Код | Назначение | Main Unit (KC868) DRAFT ресурс | Nano DRAFT ресурс | Safety note | Статус | Блокеры |
|---|---|---|---|---|---|---|
| `SEN_FLAME` | Датчик пламени (qrb1/фоторезистор по решению) | **BLK-005 PROPOSED/NOT VALIDATED:** фоторезистор → **ADC A1 (GPIO36) или A2 (GPIO39) — voltage 0–5 В каналы** (A6 official data) + экранирование + ЭМС PASS/FAIL при розжиге. **A3 (GPIO34)/A4 (GPIO35)** — режим 0–5 В vs 4–20 мА **TBD**; не использовать для voltage path до подтверждения. **Fallback:** внешний изолированный формирователь digital/Schmitt → **direct ESP32 GPIO кандидат** (TBD free GPIO + board test). | Прямого входа нет; супервизия по heartbeat/`flameStatus`/FSM + safety-gates/датчики Nano | **Отклонение от SR-B13/S-47.** Обрыв/КЗ/потеря/saturated/stuck/invalid = «нет пламени». Max latency, latch/no auto-restart. До APPROVE не реализовывать. | **BLOCKED (BLK-005, PROPOSED/NOT VALIDATED)** | BLK-005, BLK-006, `safety-reviewer`+`schematic-engineer` APPROVE |
| `SEN_AIR_PRESSURE` | Давление воздуха, пороговый | Наблюдение по данным/телеметрии (если передаётся) | D2 кандидат — **прямой Nano, инвариант** | Нет/недостоверно давления => запрет топливоподачи/опасных действий. | **FIXED-DRAFT** (Nano direct) | Схема датчика/полярность/ЭМС |
| `SEN_THERMOCOUPLE_K` | Термопара K + MAX6675 (выхлоп) | **Кандидат:** SPI socket KC868 (CS/MOSI/MISO/SCK) или иной SPI на Main Unit (TBD). Технологическая обратная связь горения. | **Не safety-зона Nano (BLK-003)** | Main Unit при fault/timeout/invalid => безопасный прикладный режим (FSM), не Nano interlock. `safety-reviewer` ACK req. | **CANDIDATE (BLK-003 ACCEPTED RISK, ACK req.)** | BLK-003, SPI/JTAG конфликт socket, диагностика MAX6675 |
| `SEN_OIL_TEMP_SAFETY` | DS18B20 аварийной отсечки масла | Использует по данным (не источник аварийной отсечки) | D11 кандидат — **прямой Nano, инвариант** (1-Wire) | Независимая защита Nano. | **FIXED-DRAFT** | Отдельная линия, защита 1-Wire |

### 3.3. Датчики — технологические (уровни/температуры)

| Код | Назначение | Main Unit (KC868) DRAFT ресурс | Nano DRAFT ресурс | Safety note | Статус |
|---|---|---|---|---|---|
| `SEN_FLOAT_MIN` | Поплавок мин. уровня, геркон | Наблюдение | D3 — **прямой Nano, инвариант** | Критический минимум; независимая защита Nano. | **FIXED-DRAFT** |
| `SEN_FLOAT_MAX` | Поплавок макс. уровня, геркон | Наблюдение | D4 — **прямой Nano, инвариант** | Отсечка подкачки; Nano block/stop контекст. | **FIXED-DRAFT** |
| `SEN_OIL_TEMP_MAIN` | DS18B20 ПИД подогрева | 1-Wire шина 1/2 (GPIO15/16 кандидат), технологический | Наблюдение при необходимости (не аварийная отсечка) | Non-safety control; аварийная отсечка по `SEN_OIL_TEMP_SAFETY` на Nano. | **CANDIDATE** |
| `SEN_TEMP_BOILER_IN` | DS18B20 обратка котла | 1-Wire технологический | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_TEMP_BOILER_OUT` | DS18B20 подача котла | 1-Wire технологический | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_TEMP_BOILER_CORE` | DS18B20 рубашка котла | 1-Wire технологический | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_TEMP_TA_IN` | DS18B20 вход ТА | 1-Wire технологический | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_TEMP_TA_OUT` | DS18B20 выход ТА | 1-Wire технологический | Наблюдение | Non-safety. | **CANDIDATE** |

### 3.4. Выносные датчики коллекторов (перспектива)

| Код | Назначение | Main Unit DRAFT ресурс | Nano | Safety note | Статус |
|---|---|---|---|---|---|
| `SEN_FLOOR_1_SUPPLY` | Подача коллектора 1 эт., DS18B20 | 1-Wire (распределение по шинам TBD) | Наблюдение | Non-safety, перспектива. | **CANDIDATE** |
| `SEN_FLOOR_1_RETURN` | Обратка коллектора 1 эт. | 1-Wire | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_FLOOR_1_REAL` | Темп. плиты пола 1 эт. | 1-Wire | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_FLOOR_B_SUPPLY` | Подача коллектора подвала | 1-Wire | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_FLOOR_B_RETURN` | Обратка коллектора подвала | 1-Wire | Наблюдение | Non-safety. | **CANDIDATE** |
| `SEN_FLOOR_B_REAL` | Темп. плиты пола подвала | 1-Wire | Наблюдение | Non-safety. | **CANDIDATE** |

### 3.5. Управляющие модули (из `rnd/peripherals.md` §3.1)

| Код | Роль | DRAFT mapping | Статус |
|---|---|---|---|
| `MAIN_UNIT` | Главный контроллер (было ESP32 NodeMCU v3 в spec) | **KC868-A6 (не v3) / ESP32-WROOM-32** (safety-gated), `src_esp32/`, `env:esp32_core` (`board = esp32dev`) | **FIXED-DRAFT** (архитектурное решение, условное) |
| `SAFETY_UNIT` | Модуль безопасности | **Arduino Nano (ATmega328P)**, `src_avr8/`, `env:arduino_safety` — **не заменяется** | **FIXED-DRAFT** |

## 4. Матрица конфликтов / рисков (DRAFT)

| Конфликт / риск | Затронутые ресурсы | Влияние | Митигация / статус |
|---|---|---|---|
| Nano D0/D1 vs USB | Safety UART / прошивка Nano | Невозможна одновременная USB-отладка Nano и safety UART без развязки/процедуры | Процедура обязательна: переключатель/разъём, обесточивание KC868 или удержание reset; фиксировать в schematic. **TBD** |
| KC868 free GPIO/strapping | A6: 12/13, 25/26, 16/17, 32/33, 21/2/22; GPIO0/2/5/12/15 strapping; 34–39 input-only | Неверное назначение => boot fail/конфликт/повреждение | **A6 официального списка free GPIO нет**; кандидаты требуют отказа от функции + board test. GPIO0 — не использовать; strapping проверить. Не фиксировать до теста. |
| ADC A3/A4 — режим TBD | `SEN_FLAME` voltage path | A6: A3=GPIO34 / A4=GPIO35 через ОУ/BAT54S; 0–5 В vs 4–20 мА **TBD**. | Для `SEN_FLAME` использовать только **A1 (GPIO36) / A2 (GPIO39)** — подтверждённые voltage-каналы. A3/A4 — после измерений. **BLK-005 всё равно BLOCKED.** |
| Общая I2C KC868 (PCF8574/RTC/OLED/DAC) | Реле, опто, сервисные устройства | Единый домен отказа; `I2C_FAULT` при «живом» heartbeat; latch PCF8574 | Не safety path. I2C health в heartbeat (BLK-006). Митигации §5.3/§7.9; Nano hardware backup обязателен. |
| Safety UART vs RS485 | UART point-to-point vs встроенный RS485 | Ошибочное использование RS485 для safety | **Запрещено** без отдельного APPROVE; safety link — отдельный UART. S-49 RS485 только non-safety. |
| BLK-005 ADC path vs SR-B13/S-47 | `SEN_FLAME` на ADC KC868 | Наводки ВВ розжига, дрейф, ложное «пламя/нет пламени», ADC raw не утверждён | **BLOCKED до валидации/APPROVE.** Экранирование, fail-safe пороги, диагностика, ЭМС PASS/FAIL; fallback на direct GPIO + изолированный формирователь. |
| SPI socket vs JTAG/LoRa/nRF vs MAX6675 | `SEN_THERMOCOUPLE_K` (BLK-003) | Выбор занимает сокет/JTAG линии; BLK-003 ACK req. | Разместить MAX6675 на Main Unit как технологический; подтвердить конфликт пинов/отладку. |
| 6 onboard реле KC868 vs 7+ прикладных нагрузок | Все актуаторы Main Unit | Дефицит/непригодность как safety path | `ACT_HEATER_OIL` на внешний SSR/driver pilot; силовая коммутация внешняя + Nano backup. Нумерация relay TBD. |
| Nano I/O budget tight | D0–D11 минимум, BLK-007 опция, диагностика | Риск нехватки при добавлении feedback/контактов | Приоритет safety; пересчёт при любом добавлении. A6/A7 analog-only. |
| SR-B6 board-off KC868 vs продувка/логика | `ACT_FAN_BLOWER` relay на KC868, FSM на KC868 | Board-off убивает управляемую продувку Main Unit | **Не штатный механизм продувки** (BLK-001). Аварийный side effect; схема поведения/автомата обязательна. |
| BLK-007 дублированное включение `ACT_PUMP_BOILER` Nano | Параллельный/альтернативный путь включения | Обратное питание, конфликт с KC868 relay, опасное поведение при отказе | **TBD schematic:** диоды/изоляция/приоритеты/автомат. |
| Общий Nano cutoff (BLK-002) single-point / weld residual | D7 group: ТЭН+подкачка+розжиг | Залипание/сварка => группа может остаться запитанной (residual risk draft ACK req.) | Не самозамыкающийся, no bypass, feedback fail-safe, latch, SR-B4 теплушка/термопредохранители, safety-gates+`SEN_AIR_PRESSURE`. **BLOCKED.** |
| Grounding/isolation §7.18 | UART, KC868 12V feedback, датчики | Шунтирование барьера, помехи, ложные состояния | Выбрать A или B до схемы; **не смешивать**. |
| 220В коммутация/ЭМС/ВВ розжиг | Все 220В актуаторы, `SEN_FLAME`, сигнальные линии | Пожар/поражение/ложные срабатывания/повреждение | S-48: DIN вне отсека котла, УЗО type A 30mA, предохранители, clearance/creepage, транзиенты, разводка, ЭМС-тесты. |

## 5. Открытые вопросы для `schematic-engineer` + `safety-reviewer`

1. **KC868-A6 board-level (схема доступна, верификация не выполнена):** сверить фактическую ревизию платы с `KC868-A6-schematic.pdf`; подтвердить свободные GPIO/strapping, тип/группировку контактов реле SPDT (ULN2003A), поведение PCF8574/реле при reset/brownout/power loss, опто-входы EL357N, ADC A1=36/A2=39/A3=34/A4=35 (A3/A4 0–5 В vs 4–20 мА), RS485 14/27 (MAX13487EESA, **без изоляции**), RS232 16/17, 1-Wire 32/33, I2C 4/15, SPI 5/18/19/23, DAC 25/26, extend serial 12/13. **Свободные GPIO официальным списком не подтверждены**; кандидаты — только при отказе от функции + board test + strapping (GPIO0/2/5/12/15; 34–39 input-only). **v3-пины к A6 не применять** (см. `kc868-a6v3_analysis.md`, архив).
2. **§7.18 grounding/isolation:** вариант A (раздельные 0В + изоляция UART/feedback) или B (общий 0В)? Зафиксировать до схемы; не смешивать.
3. **Nano D0/D1:** финальная процедура прошивки/отладки (переключатель/разъём/удержание KC868). Альтернативный канал — только с `safety-reviewer`.
4. **Safety UART:** финальные GPIO (после board test), уровни, изоляция, baud, жгут/экранирование. **BLK-006 APPROVE до прошивки.**
5. **BLK-002 общий cutoff:** устройство, AC-категория/inrush/ЭМС, driver, no self-latch/no bypass, feedback fail-safe полярности, reset/brownout/Hi-Z/WDT поведение, residual weld FMEA + ACK.
6. **SR-B4 `ACT_HEATER_OIL`:** SSR+Nano cutoff последовательно с ТЭН; теплушка на корпусе бака (место, уставка, номиналы, **тип сброса ручной/самовозврат + ACK**), термопредохранители; Hi-Z SSR pilot = safe.
7. **SR-B6:** ключ 12В KC868, soft-start/current limit, 3-state feedback, min off-time/limits/no fast cycling, weld/stuck, latch, влияние board-off на нагрузки KC868 (BLK-001/004).
8. **SR-B14/SR-B15/SR-B12:** таблица уровней Nano outputs при normal/reset/brownout/bootloader/Hi-Z/WDT/power loss; pull resistors; отпускание катушек/внешний watchdog.
9. **BLK-005 `SEN_FLAME`:** **ADC A1 (GPIO36) или A2 (GPIO39) — voltage 0–5 В; A3 (GPIO34)/A4 (GPIO35) режим TBD** vs fallback direct GPIO; фронт-энд/делитель/импеданс, обрыв/КЗ=«нет пламени», PASS/FAIL ЭМС-тесты при розжиге, max latency, latch. **APPROVE до реализации.**
10. **BLK-003 MAX6675:** KC868 SPI socket/альтернативный SPI, JTAG конфликт, диагностика. ACK границы ответственности.
11. **BLK-007 `ACT_PUMP_BOILER`:** схема дублированного включения Nano (если A0), приоритеты, автомат, диоды/изоляция обратного питания.
12. **1-Wire:** распределение DS18B20 по GPIO32/33 (A6 official data: 1-Wire 32/33), длины/экранирование/ЭМС; `SEN_OIL_TEMP_SAFETY` отдельно на Nano D11.
13. **S-48 220В:** NO/COM/NC, предохранители, УЗО type A 30mA, варисторы/snubber, clearance/creepage, DIN вне отсека котла.
14. **S-49/BLK-010 RS485:** DMX512/Modbus сегменты, терминация/bias, `DE/RE`, таймауты, slave watchdog/fail-safe defaults, изоляция.
15. **`rnd/peripherals.md` §3.1:** обновить `MAIN_UNIT` (NodeMCU v3 → KC868-A6, safety-gated). **Выполнено.**

## 6. Чек-лист финализации (открыт)

- [ ] Назначения подтверждены board test/даташитами; нет конфликтов (strapping/USB/RS485/RS232/JTAG/I2C/SPI socket). **Схема A6 доступна** (сверить ревизию) — свободные GPIO/ADC/реле-факты проверены измерением (особенно ADC A3/A4, GPIO0/2/12/15 strapping, relay contact type).
- [ ] Nano прямые sensors/safety-gates/cutoff paths подтверждены; обход Main Unit невозможен.
- [ ] Safety UART + BLK-006 утверждены `safety-reviewer`.
- [ ] BLK-002/BLK-005/BLK-007/SR-B6 **проектные** схемы (внешние силовые/защитные цепи, не внутренняя схема A6) утверждены `schematic-engineer` + `safety-reviewer`.
- [ ] SR-B14/SR-B15/отпускание катушек/внешний watchdog подтверждены стендом.
- [ ] S-48/S-49/§7.18 выполнены.
- [ ] Стендовые тесты (analysis §10, BLK-005 ЭМС, SR-B6, protocol vectors) + evidence review.
- [ ] Документ переведён из **DRAFT** в утверждённый статус **только** после gates/approvals.

---
*Конец DRAFT pinmap proposal. Не утверждено. Финализация шага 3 заблокирована до закрытия CRITICAL/HIGH условий.*

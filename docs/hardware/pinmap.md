# Пин-мап (DRAFT / PLANNING PROPOSAL — NOT APPROVED)

> **СТАТУС: ЧЕРНОВИК ПЛАНИРОВАНИЯ. НЕ УТВЕРЖДЕНО. ФИНАЛИЗАЦИЯ ШАГА 3 ЗАБЛОКИРОВАНА.**
>
> Документ **не фиксирует** окончательные назначения пинов/ресурсов и **не может** использоваться как основание для «утверждённой» схемы, разводки, прошивки safety-логики или монтажа.
>
> **Статус:** DRAFT. Это рабочий пин-мап, а не утверждённый документ: линии помечены и уточняются по мере проверки на реальной плате. CANDIDATE-линии при отладке допустимы, если понятно, что они временные.
>
> **Main Unit = KC868-A6.** Раздел §2 — **A6 (ESP32-WROOM-32)**, остаётся **DRAFT/TBD** до board test (`docs/hardware/kc868-a6_analysis.md` §7), закрытия BLK-012 и approvals.
>
> **Обязательные ревью перед утверждением/реализацией:** `schematic-engineer` + `safety-reviewer` (силовая часть 220В, изоляция/grounding §7.18, fail-safe уровни, протокол BLK-006, BLK-002/005 gates).

> **Границы документа:** физические назначения пинов. Владение периферией — `docs/hardware/periph_distribution.md`; инвентаризация линий, бюджет и дефицит A6 — `docs/hardware/kc868-a6_connection_plan.md`; статусы блокеров — `docs/architecture/blockers.md`; факты платы — `docs/hardware/kc868-a6_analysis.md`.

## Контекст / инварианты (без ослабления)

- **Safety Unit:** Arduino Nano (ATmega328P), `env:arduino_safety`, `src_avr8/` — независимый, **не заменяется**. Финальное право блокировки — Nano.
- **Main Unit (safety-gated):** KC868-A6, ESP32-WROOM-32, `env:esp32_core` (`board = esp32dev`).
- **Nano safety-gates:** `ACT_OIL_VALVE_SAFETY`, `ACT_AIR_VALVE_SAFETY` (NC) — только Nano, обхода нет.
- **Прямые датчики Nano:** `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_MIN`, `SEN_FLOAT_MAX` — только прямые входы Nano.
- **PCF8574/I2C KC868 (реле `0x24`, опто `0x22`) — не fail-safe safety path.** Onboard реле — только прикладное управление при Nano hardware backup. `SEN_FLAME` (BLK-005) — PROPOSED/NOT VALIDATED.
- **Safety link:** dedicated point-to-point UART KC868↔Nano (**не RS485**). USB-C + CH340C — debug. Nano D0/D1 конфликтуют с USB-программированием.
- **BLK-001:** `ACT_FAN_BLOWER` — реле KC868; per-circuit Nano cutoff не требуется; Nano может обесточить всю плату (SR-B6, coarse).
- **BLK-002:** общий Nano-контактор/силовое реле (не самозамыкающееся, без самоподхвата, только Nano, без шунтирования) для `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT`; блоуэр в обход.
- **BLK-003:** `SEN_THERMOCOUPLE_K`/MAX6675 — технологическая обратная связь Main Unit, не зона Nano.
- **BLK-005:** `SEN_FLAME` — фоторезистор на аналоговый вход KC868 + ЭМС-тесты; fallback — изолированный формирователь на direct GPIO. Отклонение от SR-B13/S-47, пока не реализуется.
- **BLK-006:** `include/common_protocol.h`/`docs/protocol/uart.md` safety draft; прошивка — после согласования формата с Nano.

## Легенда статусов (draft)

| Статус | Значение |
|---|---|
| **FIXED-DRAFT** | Роль/инвариант/класс зафиксирован; физический пин/клемма/номинал **не утверждён**. |
| **CANDIDATE** | Предлагаемый вариант; требует подтверждения схемой/strapping/уровнями/ревью. |
| **TBD** | Уточнение обязательно. **Не финальное.** |
| **BLOCKED** | Нельзя финализировать/утверждать/реализовывать до gate/блокера/approval. |

## 1. Safety Unit — Arduino Nano (DRAFT, NOT APPROVED)

**Платформа:** Arduino Nano (ATmega328P, 5V), `env:arduino_safety`.
**Приоритет назначения:** прямые safety sensors → safety-gates → общий cutoff driver/feedback → SR-B6 control/feedback → safety UART → диагностика/резерв.
**Ограничения:** D0/D1 конфликт с USB; A6/A7 — только ADC; Hi-Z при reset/bootloader/brownout — риск (SR-B14/SR-B15).

| Nano pin | DRAFT роль / сигнал | Периферия/функция | Тип/уровень/развязка (TBD схемой) | Fail-safe требование | Статус | Примечание |
|---|---|---|---|---|---|---|
| D0 (RX) | Safety UART RX (← KC868) | Safety link (BLK-006) | UART RX, уровни/изоляция §7.18 | Потеря/invalid ⇒ safe state/latch | **CANDIDATE** | Конфликт с USB Nano; процедура прошивки обязательна. |
| D1 (TX) | Safety UART TX (→ KC868) | Safety link | UART TX | Idle/Hi-Z не создаёт ложный permit | **CANDIDATE** | См. D0. |
| D2 | `SEN_AIR_PRESSURE` input | Прямой Nano, инвариант | Digital threshold, развязка/фильтр/подтяжка TBD | Нет/недостоверно давление ⇒ запрет топлива | **CANDIDATE** | Подтвердить полярность/дребезг/ЭМС/обрыв/КЗ. |
| D3 | `SEN_FLOAT_MIN` input | Прямой Nano, инвариант | Геркон, подтяжка/фильтр TBD | Критический минимум/неопределённость ⇒ блокировка | **CANDIDATE** | Подтвердить NC/NO, fail-safe уровень. |
| D4 | `SEN_FLOAT_MAX` input | Прямой Nano, инвариант | Геркон | Max/неопределённость ⇒ block/stop подкачки | **CANDIDATE** | SR-B18 контекст. |
| D5 | `ACT_OIL_VALVE_SAFETY` driver | Только Nano, NC safety-gate | Output→драйвер→независимая цепь; NO relay energized-to-open / гидравлика NC TBD | Авария/timeout/latch/reset/brownout/Hi-Z/WDT ⇒ закрыт | **CANDIDATE** | Инвариант. Обход Main Unit невозможен. S-48. |
| D6 | `ACT_AIR_VALVE_SAFETY` driver | Только Nano | Аналогично D5 | Закрыт при unsafe | **CANDIDATE** | Инвариант. |
| D7 | `ACT_GROUP_CUTOFF` driver | Общий cutoff `ACT_HEATER_OIL`+`ACT_PUMP_OIL_TRANSFER`+`ACT_IGNITION_UNIT` (BLK-002) | Output→драйвер катушки; de-energize-to-trip TBD | Reset/brownout/Hi-Z/WDT/потеря питания ⇒ разомкнут/обесточен | **CANDIDATE (BLOCKED BLK-002)** | Не самозамыкающийся, без самоподхвата/шунтирования, только Nano. Блоуэр в обход. |
| D8 | `ACT_GROUP_CUTOFF_FB` | Feedback общего cutoff (BLK-002) | Вход feedback, тип TBD | Обрыв/КЗ/неизвестность = `UNKNOWN/FAULT`, не `OFF_CONFIRMED` | **CANDIDATE (BLOCKED BLK-002)** | Aux/mirror/voltage sense — схема; не шунтирует силовую цепь. |
| D9 | `KC868_12V_CTRL` | SR-B6 supervised power-cycle 12В KC868 | Output→силовой ключ 12В + soft-start/current limit TBD | Nano reset/brownout/loss/WDT ⇒ отпускание по схеме; своё питание не трогает | **CANDIDATE (BLOCKED SR-B6/BLK-008)** | Last resort, no fast cycling, min off-time/limits, latch. |
| D10 | `KC868_12V_FB` | 3-state feedback питания KC868 | Вход `present/absent/fault-unknown` TBD | Обрыв/отказ = `fault/unknown`, не подтверждение отсечки | **CANDIDATE (BLOCKED SR-B6/§7.18)** | Изоляция при раздельных 0В. |
| D11 | `SEN_OIL_TEMP_SAFETY` 1-Wire | Прямой Nano, инвариант (DS18B20) | 1-Wire + подтяжка/защита/экранирование TBD | Timeout/fault/invalid ⇒ консервативная защита | **CANDIDATE** | Отдельная линия от DS18B20 Main Unit. |
| D12 | Резерв/сервис | Non-safety (TBD) | TBD | Hi-Z не опасен | **TBD** | Не расходовать без пересчёта бюджета. |
| D13 | Резерв/индикация | Non-safety | TBD | Non-safety | **TBD** | — |
| A0 | `ACT_PUMP_BOILER` Nano enable (опция) | BLK-007 дублированное включение (если схемой подтверждено) | Output→драйвер/цепь, обратное питание/изоляция TBD | Hi-Z/de-energized = не включает | **TBD (BLK-007 schematic)** | Только по схеме; иначе резерв. |
| A1–A5 | Резерв/диагностика | — | Digital/ADC TBD | Non-safety | **TBD** | A4/A5 — I2C ATmega328P (не планируется). |
| A6, A7 | Резерв (ADC only) | — | **Analog input only** | — | **TBD** | **Не digital I/O.** |

### Бюджет I/O Nano

| Категория | Кол-во | Пины (кандидат) | Статус |
|---|---:|---|---|
| Прямые safety sensors | 4 | D2, D3, D4, D11 | FIXED-DRAFT инварианты |
| Safety-gate outputs | 2 | D5, D6 | FIXED-DRAFT |
| Общий cutoff driver+feedback | 2 | D7, D8 | CANDIDATE, **BLK-002** |
| SR-B6 control+feedback | 2 | D9, D10 | CANDIDATE, **SR-B6/BLK-008** |
| Safety UART | 2 | D0, D1 | CANDIDATE, конфликт USB |
| BLK-007 опция `ACT_PUMP_BOILER` | 0–1 | A0? | TBD |
| Резерв | 9 | D12, D13, A1–A5, A6–A7 | часть ADC-only; бюджет tight при расширении |
| **Минимум (без BLK-007)** | **12** | D0–D11 | — |

**Митигации дефицита:** MAX6675 — на Main Unit (BLK-003), не на Nano; внешний watchdog — схемно; любой доп. контакт — пересчёт + `safety-reviewer`.

## 2. Main Unit — KC868-A6 (DRAFT, NOT APPROVED)

**Платформа:** KC868-A6, ревизия v1.3, ESP32-WROOM-32, USB-C + CH340C, DIN/12В. `env:esp32_core`, `board = esp32dev`.
**Safety constraint:** onboard реле/опто-входы через PCF8574/I2C — не fail-safe safety path. Реле — только прикладное при Nano hardware backup.
**GPIO note:** официального списка свободных GPIO нет. Кандидаты требуют отказа от штатной функции + board test. GPIO34–39 input-only; GPIO0/2/5/12/15 — strapping.

### 2.1. Кандидатные назначения onboard реле (PCF8574 `0x24`, не safety path)

| Канал | Нагрузка | Контакт | Nano hardware backup | Статус |
|---:|---|---|---|---|
| ch0 | `ACT_FAN_BLOWER` | NO | BLK-001: board-off всей KC868 (SR-B6) + автомат; per-circuit cutoff не требуется | **CANDIDATE (BLK-001/SR-B6 board test)** |
| ch1 | `ACT_OIL_VALVE_MAIN` | NO | `ACT_OIL_VALVE_SAFETY` NC только Nano последовательно | **CANDIDATE** |
| ch2 | `ACT_AIR_VALVE_MAIN` | NO | `ACT_AIR_VALVE_SAFETY` NC только Nano последовательно | **CANDIDATE** |
| ch3 | `ACT_IGNITION_UNIT` | NO | В группе общего Nano cutoff (BLK-002) | **CANDIDATE (BLK-002 gate)** |
| ch4 | `ACT_PUMP_OIL_TRANSFER` | NO | SR-B18 + BLK-002: Nano hardware block/stop, fail-safe OFF | **CANDIDATE (BLK-002/SR-B18)** |
| ch5 | `ACT_PUMP_BOILER` | NO/NC — TBD | BLK-007: возможное (не по умолчанию) дублированное включение Nano | **CANDIDATE (BLK-007 schematic)** |

> Поведение PCF8574/ULN2003A/реле при POR/reset/brownout/power loss **не подтверждено** — до board test нельзя считать реле гарантированно разомкнутым.
>
> **`ACT_HEATER_OIL` — не onboard реле:** внешний SSR/драйвер pilot (direct GPIO + pull-down, Hi-Z = выключено) + Nano series cutoff + теплушка. Статус **BLOCKED (BLK-002)**.

### 2.2. Кандидаты direct GPIO / аналоговых линий A6

| Функция | Кандидат A6 | Направление/режим | Ограничение | Статус |
|---|---|---|---|---|
| Safety UART TX → Nano | **GPIO12** (extend serial TXD) | Выход, UART TX + внешний **pull-down** | **Strapping (MTDI):** только TX; boot test обязателен; как RX не использовать | **CANDIDATE (board test)** |
| Safety UART RX ← Nano | **GPIO13** (extend serial RXD) | Вход, UART RX | Не strapping; Nano TX idle high допустим | **CANDIDATE (board test)** |
| `ACT_HEATER_OIL` SSR pilot | **GPIO25** (DAC2), альтернатива GPIO26 | Digital out | Проверить, что LM258DR не мешает цифровому уровню; pull-down; BLK-002 | **CANDIDATE (board test)** |
| `SEN_FLAME` основной | **ADC A1 = GPIO36** | Analog 0–5 В | Input-only; только после проверки BLK-005; обрыв/КЗ = «нет пламени» | **кандидат (BLK-005)** |
| `SEN_FLAME` fallback | **ADC A2 = GPIO39** (как digital) | Digital input (изолированный формирователь) | Input-only; учесть делитель 0–5 В/импеданс | **BLOCKED (BLK-005)** |
| `SEN_THERMOCOUPLE_K` + MAX6675 | SPI: CS5 / SCK18 / MISO19 / MOSI23 | SPI | BLK-003 (риск принят); GPIO5 strapping; сокет не занимать LoRa/nRF | **CANDIDATE** |
| Технологические DS18B20 | 1-Wire 1 = GPIO32, 1-Wire 2 = GPIO33 | 1-Wire | Подтяжка/экран/разводка; `SEN_OIL_TEMP_SAFETY` не здесь | **CANDIDATE** |
| **Дозирующий насос STEP (PROPOSED)** | **GPIO21** (LoRa RST) | Digital out, hardware timer/RMT | Только direct GPIO, не PCF8574; BLK-002 scope extension | **CANDIDATE (board test)** |
| **Дозирующий насос DIR (PROPOSED)** | **GPIO22** (nRF CE) | Digital out | Фиксировать логику направления; BLK-002 scope | **CANDIDATE (board test)** |
| **Дозирующий насос ENA (PROPOSED)** | Аппаратно/Nano cutoff; опция GPIO26 | Hardware enable | Не расходовать GPIO, если можно; питание драйвера — Nano cutoff | **TBD** |
| Детект 220В | Опто-вход `0x22` или ADC A3/A4 | Discrete/analog | A3/A4 режим 0–5 В vs 4–20 мА TBD; non-safety | **TBD** |
| Сервис/индикация | Резерв 16/17/2/26 | — | Приоритет — safety UART/STEP; лучше USB/лог | **TBD** |

> **Прошивка:** GPIO21/22 — дефолтные SDA/SCL для `Wire`. При инициализации I2C **явно задавать GPIO4/15**.
>
> Детали бюджета, конфликтов и сценариев дефицита — `docs/hardware/kc868-a6_connection_plan.md`.

### 2.3. Safety UART link (KC868 ↔ Nano) — placeholders (BLK-006 / §7.18)

| Параметр | DRAFT значение / placeholder | Статус | Примечание |
|---|---|---|---|
| Топология | Dedicated point-to-point UART, полнодуплекс (кандидат) | **FIXED-DRAFT** | Не RS485; не общий сегмент S-49. |
| KC868 side | **GPIO12 TX (pull-down) / GPIO13 RX** | **CANDIDATE (board test)** | USB-C/CH340C не safety. GPIO12 — strapping. |
| Nano side | D0 RX / D1 TX | **CANDIDATE** | Конфликт с USB Nano; процедура прошивки обязательна. |
| Уровни | 5V (Nano) ↔ 3.3V (ESP32), двустороннее согласование | **TBD** | ESP32 не 5V tolerant; трансляторы ≠ изоляция. |
| Изоляция | По архитектуре §7.18 (A или B), **не смешивать** | **BLOCKED (§7.18/schematic)** | При раздельных 0В — digital isolator/оптопары. |
| Скорость | Единая, ≤19200 бод | **TBD (после BLK-006/тестов)** | Текущие 115200/9600 — расхождение, миграция. |
| Формат/CRC/seq/protoVer | `include/common_protocol.h` safety draft | **BLOCKED (BLK-006)** | CRC/endianness/packing/test vectors/unknown handling — до прошивки. |
| Таймауты | Heartbeat 100–200 мс, timeout ≤3 периодов (≤600 мс) | **FIXED-DRAFT** (норматив) | Обе стороны fail-safe. |
| Разъём/жгут | TBD (экранирование, длина, отвод от 220В/ВВ розжига) | **TBD** | S-48/ЭМС. |

## 3. Чек-лист финализации (открыт)

- [ ] **BLK-012 board test:** ревизия vs официальная схема A6; MCU/flash/PSRAM; PCF8574/реле при POR/reset/brownout/power loss; контакты SPDT; опто-входы EL357N; ADC A1–A4; свободные GPIO/strapping; RS485 без изоляции.
- [ ] Nano прямые датчики/safety-gates/cutoff paths подтверждены; обход Main Unit невозможен.
- [ ] Safety UART + BLK-006 утверждены `safety-reviewer`.
- [ ] BLK-002/BLK-005/BLK-007/SR-B6 схемы (внешние силовые/защитные цепи) утверждены `schematic-engineer` + `safety-reviewer`.
- [ ] SR-B14/SR-B15: уровни Nano outputs при reset/brownout/Hi-Z/WDT/bootloader; отпускание катушек; внешний watchdog — стенд.
- [ ] S-48/S-49/§7.18 выполнены.
- [ ] Стендовые тесты (analysis §7, BLK-005 ЭМС, SR-B6 ramp/weld/feedback, protocol vectors) + evidence review.
- [ ] Документ переведён из **DRAFT** в утверждённый статус **только** после gates/approvals.

## Ссылки
- `docs/hardware/periph_distribution.md` — владение периферией
- `docs/hardware/kc868-a6_connection_plan.md` — инвентаризация/бюджет/дефицит линий A6
- `docs/hardware/kc868-a6_analysis.md` — факты платы, §6 условия, §7 board test
- `docs/architecture/blockers.md` — BLK-001…BLK-012, gates
- `docs/architecture/modules.md` — роли и принципы
- `docs/protocol/uart.md`, `include/common_protocol.h` — safety UART (BLK-006)
- `docs/safety/checklist.md`, `docs/safety/blk002_closure_package.md`, `docs/safety/fsm_safety_boundary.md`
- `rnd/peripherals.md` — перечень периферии

---
*Конец DRAFT pinmap proposal. Не утверждено. Финализация шага 3 заблокирована до закрытия CRITICAL/HIGH условий.*

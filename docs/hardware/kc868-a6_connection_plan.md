# План подключений KC868-A6 (DRAFT / NOT APPROVED)

> **СТАТУС: ПРЕДВАРИТЕЛЬНЫЙ ПЛАН. НЕ УТВЕРЖДЕНО. НЕ ЯВЛЯЕТСЯ PINMAP/SCHEMATIC APPROVAL.**
>
> Документ отвечает на вопрос: **к каким линиям KC868-A6 предварительно планируется подключить периферию, какие линии свободны, каких линий не хватает.**
>
> Плата: **KC868-A6, ревизия v1.3**, ESP32-WROOM-32, USB-C + CH340C. Дата: 2026-11-10.
>
> **Границы документа:** бюджет и дефицит линий A6. Это **не** замена `docs/hardware/pinmap.md` (окончательные назначения — там) и **не** схема подключения/основание для монтажа. Все назначения — **CANDIDATE/TBD** до board test (`docs/hardware/kc868-a6_analysis.md` §7), закрытия **BLK-012** и approvals `schematic-engineer` + `safety-reviewer`. Safety Unit **Arduino Nano не заменяется**.
>
> **Ключевой вывод:** входов у A6 достаточно, **реле — ровно 6/6 без запаса**. Подтверждённо свободных выходных GPIO нет, но с учётом отказа от **RS232** и **LoRa/nRF** резерв появляется: **GPIO21/22** — лучшие «чистые» кандидаты, **GPIO16/17** — с оговоркой по SP3232EEN, **GPIO12** — strapping, **GPIO25/26** — DAC. **Дозирующий насос (шаговый драйвер) требует 2 direct GPIO (STEP/DIR) + обязательного hardware cutoff со стороны Nano; на PCF8574/I2C его вешать нельзя.**

---

## Легенда статусов

| Статус | Значение |
|---|---|
| **FIXED-DRAFT** | Роль/инвариант зафиксирован архитектурно; физическая линия/клемма не утверждена. |
| **CANDIDATE** | Предлагаемая линия; требует board test/схемы/ревью. |
| **FREE-PLANNED** | Пользователь заявил, что штатная функция (RS232/LoRa/nRF) не будет использоваться; линия считается свободной **для планирования**, но формально не утверждена и требует board test/фиксации. |
| **TBD** | Уточнение обязательно; не финальное. |
| **BLOCKED** | Нельзя финализировать/реализовывать до закрытия блокера/approval. |

---

## 1. Инвентаризация линий A6

### 1.1. GPIO ESP32-WROOM-32 на плате A6

| GPIO | Штатная функция A6 / источник | Тип | По умолчанию | Возможное использование в проекте | Статус |
|---|---|---|---|---|---|
| 0 | Boot strapping | Strapping | Boot | **Не использовать** | — |
| 1 | UART0 TX → CH340C | Debug UART | Занят | Только прошивка/лог | **FIXED-DRAFT** (занят) |
| 2 | LoRa DIO0 | Сокет + **strapping** | **FREE-PLANNED** (LoRa не планируется) | **Не использовать** из-за strapping; резерв | **FREE-PLANNED (не рекомендуется)** |
| 3 | UART0 RX ← CH340C | Debug UART | Занят | Только прошивка/лог | **FIXED-DRAFT** (занят) |
| 4 | I2C SDA | I2C | Занят | PCF8574 `0x22/0x24`, сервис | **FIXED-DRAFT** (занят) |
| 5 | SPI CS | Сокет + **strapping** | Занят сокетом | MAX6675 CS / только при отказе от сокета | **TBD** |
| 6–11 | Internal SPI flash | Недоступны | Заняты | **Не использовать** | — |
| 12 | Extend serial TXD | UART + **strapping (MTDI)** | Условно свободен | **Кандидат №1 под safety UART TX**; проверить boot | **CANDIDATE (board test)** |
| 13 | Extend serial RXD | UART (MTCK/JTAG) | Условно свободен | **Кандидат №1 под safety UART RX** | **CANDIDATE (board test)** |
| 14 | RS485 RXD | RS485 | Занят | Non-safety RS485 | **FIXED-DRAFT** (занят) |
| 15 | I2C SCL | I2C + **strapping (MTDO)** | Занят | PCF8574, сервис | **FIXED-DRAFT** (занят) |
| 16 | RS232 RXD | RS232 + SP3232EEN | **FREE-PLANNED** (RS232 не планируется) | **Не использовать как выход без доработки:** SP3232EEN может драйвить линию | **FREE-PLANNED (board test)** |
| 17 | RS232 TXD | RS232 + SP3232EEN | **FREE-PLANNED** (RS232 не планируется) | Кандидат-выход; нагрузка — вход SP3232EEN; проверить | **FREE-PLANNED (board test)** |
| 18 | SPI SCK | Сокет | Занят сокетом | MAX6675 SCK | **TBD** |
| 19 | SPI MISO | Сокет | Занят сокетом | MAX6675 MISO | **TBD** |
| 21 | LoRa RST | Сокет | **FREE-PLANNED** (LoRa не планируется) | **Кандидат №1 под STEP дозирующего насоса** | **FREE-PLANNED (board test)** |
| 22 | nRF CE | Сокет | **FREE-PLANNED** (nRF не планируется) | **Кандидат №1 под DIR дозирующего насоса** | **FREE-PLANNED (board test)** |
| 23 | SPI MOSI | Сокет | Занят сокетом | MAX6675 MOSI | **TBD** |
| 25 | DAC2 | DAC + LM258DR | Аналоговый канал | **Кандидат под SSR pilot**; проверить цифровой режим | **CANDIDATE (board test)** |
| 26 | DAC1 | DAC + LM258DR | Аналоговый канал | **Кандидат под SSR pilot / резерв**; проверить цифровой режим | **CANDIDATE (board test)** |
| 27 | RS485 TXD | RS485 | Занят | Non-safety RS485 | **FIXED-DRAFT** (занят) |
| 32 | 1-Wire 1 | 1-Wire | Занят | Технологические DS18B20 | **CANDIDATE** |
| 33 | 1-Wire 2 | 1-Wire | Занят | Вторая шина DS18B20 / резерв при объединении | **CANDIDATE** |
| 34 | ADC A3 (input-only) | ADC/IN | Вход | Только вход; режим 0–5 В vs 4–20 мА TBD | **TBD** |
| 35 | ADC A4 (input-only) | ADC/IN | Вход | Только вход; режим TBD | **TBD** |
| 36 | ADC A1 (input-only) | ADC/IN | Вход | **Основной кандидат `SEN_FLAME`** (voltage 0–5 В) | **BLOCKED (BLK-005)** |
| 39 | ADC A2 (input-only) | ADC/IN | Вход | Резерв/второй кандидат `SEN_FLAME`; digital input для fallback | **BLOCKED (BLK-005)** |

**Недоступны на ESP32-WROOM-32:** GPIO20, 24, 28–31, 37–38.
**Strapping-пины:** GPIO0, 2, 5, 12, 15 — любое назначение только после проверки boot.
**Input-only:** GPIO34–39 — не могут управлять SSR/реле/UART TX; на A6 к ним подключён аналоговый фронт.

### 1.2. Сводка: что реально свободно

| Категория | Линии | Комментарий |
|---|---|---|
| **Подтверждённо свободные** | **нет** | Официального списка свободных GPIO у A6 нет. |
| **FREE-PLANNED (освобождаются решением пользователя)** | **GPIO21, GPIO22** (LoRa/nRF), **GPIO16, GPIO17** (RS232), **GPIO2** (LoRa DIO0) | LoRa/nRF и RS232 **не планируются**. **21/22 — лучшие «чистые» кандидаты**; **16/17 — с оговоркой SP3232EEN**; **2 — strapping**. Решение требует фиксации + board test. |
| **Условно свободные (отказ от extend serial)** | **GPIO12, GPIO13** | GPIO12 — strapping (MTDI); лучшие кандидаты под safety UART, только после board test. |
| **Свободны только при отказе от функции** | GPIO25/26 (DAC), GPIO32/33 (1-Wire) | Каждая линия — отказ от штатного интерфейса. |
| **Заняты под обязательные функции** | GPIO5/18/19/23 (SPI/MAX6675), GPIO32/33 (1-Wire), GPIO4/15 (I2C), GPIO14/27 (RS485) | Пересматривать только при отказе от функции. |
| **Только входы** | GPIO34, 35, 36, 39 | ADC/input-only; аналоговый фронт. |
| **Недоступны/заняты критично** | GPIO0/1/3 (boot/UART0/CH340), GPIO6–11 (flash) | Не расходовать. |

> **Практический вывод:** при сохранении всех штатных функций — **0 подтверждённых свободных выходных GPIO**. После отказа от RS232 и LoRa/nRF появляется резерв **21/22 (чистые), 16/17 (SP3232EEN), 2 (strapping)**. Этого достаточно для safety UART + STEP/DIR + SSR pilot, но **каждая линия имеет оговорку**.
>
> ⚠️ **Критично для safety UART:** GPIO12 — strapping (MTDI) и требует **pull-down** на boot; как UART **RX** его использовать нельзя (Nano TX idle high может сорвать загрузку). Рабочий вариант — **GPIO12 = TX с pull-down, GPIO13 = RX**. Если board test не подтвердит GPIO12 даже в TX, **чистой пары GPIO под safety UART нет** — блокер, требующий схемной доработки или пересмотра платы.

---

## 2. Бюджет линий и дефицит

### 2.1. Бюджет и назначение реле: 6 нужно / 6 есть / 0 запаса

| № | Нагрузка (код) | Реле A6 | Контакт | Условие/блокер | Статус |
|---:|---|---:|---|---|---|
| 1 | `ACT_FAN_BLOWER` | 1 | NO | BLK-001: per-circuit Nano cutoff не требуется; SR-B6 board-off; POR board test | **CANDIDATE** |
| 2 | `ACT_OIL_VALVE_MAIN` | 1 | NO | `ACT_OIL_VALVE_SAFETY` NC только Nano последовательно; snubber | **CANDIDATE** |
| 3 | `ACT_AIR_VALVE_MAIN` | 1 | NO | `ACT_AIR_VALVE_SAFETY` NC только Nano последовательно | **CANDIDATE** |
| 4 | `ACT_IGNITION_UNIT` | 1 | NO | В группе общего Nano cutoff (BLK-002); ЭМС ВВ розжига | **CANDIDATE** |
| 5 | `ACT_PUMP_OIL_TRANSFER` | 1 | NO | Nano hardware block/stop OFF (BLK-002/SR-B18) | **CANDIDATE** |
| 6 | `ACT_PUMP_BOILER` | 1 | NO/NC — TBD | BLK-007: политика «оставание ON»/выбег; обратное питание/изоляция | **CANDIDATE** |
| — | **Итого onboard реле** | **6/6** | — | **Свободных реле нет.** | — |
| 7 | `ACT_HEATER_OIL` | **0** | Внешний SSR/драйвер | Не onboard relay как силовой путь; Nano series cutoff + теплушка | **BLOCKED (BLK-002)** |
| 8 | **Дозирующий насос (PROPOSED)** | **0** | STEP/DIR + внешний драйвер | Реле A6 не используется; питание драйвера — Nano cutoff | **PROPOSED** |
| — | `ACT_OIL/AIR_VALVE_SAFETY` | **0** | Только Nano | На A6 не подключаются | — |
| — | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` | **0** | RS485 slave | Non-safety, S-49 | — |

**Вывод:** 6 реле покрывают ровно 6 прикладных нагрузок. **Седьмая (`ACT_HEATER_OIL`) — внешний SSR/драйвер; дозирующий насос реле не занимает.** Любая дополнительная 220В-нагрузка — внешнее DIN-реле/контактор; **параллелить контакты нельзя**, реле A6 не safety path.

> ⚠️ **NO — схемный выбор, а не гарантия fail-safe.** Поведение PCF8574/ULN2003A/реле при POR/reset/brownout/power loss **не подтверждено**; safety-функции остаются на Nano.

### 2.2. Бюджет и назначение direct GPIO

| Функция | Линия A6 | Направление/режим | Условие/блокер | Статус |
|---|---|---|---|---|
| Safety UART TX → Nano | **GPIO12** (extend serial TXD) | Выход, UART TX + внешний **pull-down** | **Strapping (MTDI):** boot test обязателен; как RX не использовать; BLK-006 | **CANDIDATE** |
| Safety UART RX ← Nano | **GPIO13** (extend serial RXD) | Вход, UART RX | Не strapping; Nano TX idle high допустим; BLK-006; §7.18 | **CANDIDATE** |
| `ACT_HEATER_OIL` SSR pilot | **GPIO25** (DAC2), альтернатива GPIO26 | Digital out | Проверить LM258DR; pull-down; Hi-Z = safe; BLK-002 | **CANDIDATE** |
| `SEN_FLAME` основной | **ADC A1 = GPIO36** | Analog 0–5 В | Только после проверки BLK-005; экран/ЭМС; обрыв/КЗ = «нет пламени» | **кандидат (BLK-005)** |
| `SEN_FLAME` fallback | **ADC A2 = GPIO39** | Digital input (изолированный формирователь) | Input-only; учесть делитель/импеданс | **BLOCKED (BLK-005)** |
| `SEN_THERMOCOUPLE_K` + MAX6675 | SPI: CS5 / SCK18 / MISO19 / MOSI23 | SPI | BLK-003 (риск принят); GPIO5 strapping; сокет не занимать LoRa/nRF | **CANDIDATE** |
| Технологические DS18B20 | 1-Wire 1 = GPIO32, 1-Wire 2 = GPIO33 | 1-Wire | Подтяжка/экран/разводка; `SEN_OIL_TEMP_SAFETY` не здесь | **CANDIDATE** |
| Детект 220В | Опто-вход `0x22` ch0 (или ADC A3/A4) | Discrete 12 В | Внешний 220В→12В формирователь; non-safety | **TBD** |
| Сервис/индикация | Резерв 16/17/2/26 | — | Приоритет — safety UART/STEP; лучше USB/лог | **TBD** |
| **Дозирующий насос: STEP** | **GPIO21** (LoRa RST) | Digital out, hardware timer/RMT | Только direct GPIO, не PCF8574/I2C; BLK-002 scope | **CANDIDATE** |
| **Дозирующий насос: DIR** | **GPIO22** (nRF CE) | Digital out | Направление/реверс; BLK-002 scope | **CANDIDATE** |
| **Дозирующий насос: ENA** | Аппаратно/Nano cutoff; опция GPIO26 | Hardware enable | Не расходовать GPIO, если можно; питание драйвера — Nano cutoff | **TBD** |
| **Итого обязательных выходов** | — | **4** (UART TX + SSR + STEP + DIR) | 12 + 25 + 21 + 22 | **0 подтверждено** |
| **Итого обязательных входов** | — | **1** (UART RX) + опционально flame/220В | 13, 39, опто | **0 подтверждено** |

**Расчёт бюджета (с учётом отказа от RS232 и LoRa/nRF):**

- **Базовый P1:** UART — **12 (TX) / 13 (RX)**; STEP/DIR — **21/22**; SSR pilot — **25**; ENA — **аппаратно/Nano cutoff** (GPIO26 только при необходимости). Итого **4 обязательных выхода + 0–1 опциональный**; резерв — **16/17, 2, 26**.
- **Если GPIO25/26 не работают как digital:** SSR — **GPIO17** (если SP3232EEN не мешает) или **GPIO16** после доработки; ENA — аппаратно.
- **Если GPIO21/22 недоступны:** STEP/DIR — **16/17** (после доработки/отключения SP3232EEN) или **17 + 2**; GPIO2 — крайний вариант.
- **Если GPIO12 не проходит boot:** UART — **13 + 17** (проверить SP3232EEN) или **13 + 26**; 25/26 как UART RX сомнительны из-за LM258DR. Иначе — схемная доработка/пересмотр платы.
- **Если нужен ещё сервисный выход:** **16/17/2/26**; не отнимать у safety UART/STEP.
- **Входы:** 4 ADC/input-only + 6 опто-входов — для non-safety задач достаточно. Дефицита входов нет.

> **Вывод по дефициту:** после отказа от RS232/LoRa/nRF дефицит выходов снимается (4 обязательных + резерв), но **«чистых» линий всего две — GPIO21/22**. Остальные имеют ограничение: GPIO12 — strapping, GPIO25/26 — DAC, GPIO16/17 — SP3232EEN, GPIO2 — strapping. Бюджет **не подтверждён** board test.

### 2.3. Итоговая оценка

| Ресурс | Оценка |
|---|---|
| Реле | **6/6, запаса нет**; ТЭН — внешний SSR; дозирование — STEP/DIR без реле A6. |
| Direct outputs | **4 обязательных** (UART TX, SSR, STEP, DIR) + 0–1 ENA; бюджет сходится, «чистых» линий мало. |
| Direct inputs | Достаточно (ADC/input-only + опто). |
| I2C | Занята PCF8574; расширяема, но **не safety path**. |
| RS485 | Есть, но **без изоляции** и только non-safety. |
| Safety UART | Подтверждённых линий нет; базовый вариант — GPIO12/13 (12 — strapping) + board test. |

---

## 3. Предварительный план P1 (baseline)

> Согласованный по смыслу, но **не утверждённый** вариант. Минимизирует отказ от функций: extend serial под safety UART, линии LoRa/nRF 21/22 под STEP/DIR, один DAC под SSR, SPI-сокет под MAX6675, обе шины 1-Wire под DS18B20.

### 3.1. Что НЕ подключается к A6 (инварианты Nano)

| Функция | Куда | Почему |
|---|---|---|
| `ACT_OIL_VALVE_SAFETY` (NC) | **Только Nano (D5)** | Независимый safety-gate; Main Unit не управляет. |
| `ACT_AIR_VALVE_SAFETY` (NC) | **Только Nano (D6)** | Независимый safety-gate. |
| `SEN_AIR_PRESSURE` | **Прямой Nano (D2)** | Критичный датчик; не через PCF8574/I2C. |
| `SEN_OIL_TEMP_SAFETY` (DS18B20) | **Прямой Nano (D11)** | Независимая аварийная отсечка. |
| `SEN_FLOAT_MIN` | **Прямой Nano (D3)** | Критический минимум. |
| `SEN_FLOAT_MAX` | **Прямой Nano (D4)** | Отсечка подкачки. |
| Общий Nano cutoff (ТЭН/подкачка/розжиг; + дозирование — BLK-002 scope) | **Nano (D7/D8)** | BLK-002; hardware, no bypass. |
| SR-B6 power-cycle 12В A6 | **Nano (D9/D10)** | Только Nano; A6 не управляет своим отключением. |

### 3.2. Non-safety интерфейсы

| Интерфейс | Линия | Использование | Статус |
|---|---|---|---|
| RS485 | GPIO14/27 | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` (slave), S-49 | **CANDIDATE** |
| RS232 | GPIO16/17 | **Не планируется**; резерв (SP3232EEN caveat); не safety | **FREE-PLANNED (board test)** |
| I2C | GPIO4/15 | PCF8574 `0x22/0x24`, сервис | **FIXED-DRAFT** |
| USB-C/CH340C | UART0 | Прошивка/отладка/лог; **не safety link** | **FIXED-DRAFT** |
| LoRa/nRF сокет | GPIO21/2/22 + SPI | **Не планируется**; 21/22 — **STEP/DIR**, 2 — strapping | **FREE-PLANNED** |
| DAC второй канал | GPIO26 | Резерв (SSR/ENA/0–10 В привод) | **TBD** |

### 3.3. Дозирующий насос подачи масла (PROPOSED, код TBD)

> **Статус: PROPOSED / NOT IN SOURCE OF TRUTH.** Пользователь заявил о возможном добавлении дозирующего насоса с шаговым двигателем (2026-11-10). В `rnd/peripherals.md` такого актуатора нет; код `ACT_PUMP_OIL_DOSING` — предложение. Нужно уточнить: **дополнение** к `ACT_PUMP_OIL_TRANSFER` или его **замена**.

| Параметр | Предварительно | Статус/условие |
|---|---|---|
| Управление от A6 | **STEP = GPIO21**, **DIR = GPIO22**, ENA — аппаратно/Nano (опция GPIO26) | **CANDIDATE**; board test 21/22; не PCF8574/I2C. |
| Формирование импульсов | Hardware timer / RMT / LEDC, **только direct GPIO** | Частота/скважность/ускорение — TBD по драйверу и насосу. |
| Драйвер | Внешний шаговый драйвер (A4988/DRV8825/TMC или иной) | Не onboard; питание/защита/ЭМС — схема. |
| Силовое питание драйвера | **Через Nano cutoff / safety relay последовательно** | **Не** через реле A6; STEP/DIR — только управление, не safety. |
| ENA | Аппаратная цепь/Nano; при потере питания драйвер обесточен | Если нужен GPIO A6 — 26 (DAC), приоритет — не расходовать. |
| Safety-инварианты | `SEN_FLOAT_MIN/MAX`, `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, safety-gates — Nano | Дозирование не обходит Nano. |
| BLK-002 | **Расширение scope:** дозирующий насос — топливный актуатор | Требуется `safety-reviewer` + FMEA/остаточный риск. |
| Отказ STEP | Нет импульсов → нет подачи; залипание/помеха STEP → риск | Hardware cutoff питания драйвера обязателен; ПО не защита. |
| Idle/Hi-Z (reset/boot/brownout A6) | STEP/DIR без импульсов; **ENA = disabled** при Hi-Z/обесточивании; внешние pull-down/pull | **Обязательно схемой**; проверить board test. |
| Потеря heartbeat/связи | Nano → safe state → группа BLK-002 обесточена → драйвер без питания; **no auto-resume** | Latch/no auto-resume; согласовать с `docs/protocol/uart.md`. |
| ENA отдельной линией от Nano | Пересчёт бюджета Nano (`pinmap.md` §1) + `safety-reviewer` | Резерв D12/D13 — единственный. |
| DIR default | Внешний pull задаёт безопасное направление; при reset/дрейфе линию не «переворачивать» | FMEA: реверс/сифонирование при обесточенном драйвере. |
| ЭМС STEP/DIR | Короткие экранированные линии, драйвер рядом с A6, отвод от ВВ розжига/220В/силовых цепей двигателя | Включить в FMEA; совместно с BLK-005. |
| Монтаж | 220В/DC драйвер, предохранители, изоляция, S-48 | Схема/ревью. |

**Вывод:** дозирующий насос **не создаёт дефицита реле**, но требует **2 direct GPIO (STEP/DIR)** и **обязательного hardware cutoff Nano**. Расширяет BLK-002; требует обновления `rnd/peripherals.md` + `safety-reviewer` до реализации.

---

## 4. Нехватка линий: сценарии и митигации

| Сценарий | Митигация | Ограничение |
|---|---|---|
| **Базовый P1:** UART 12/13, STEP/DIR 21/22, SSR 25, ENA аппаратно/Nano | Выходов хватает; резерв 16/17/2/26 | GPIO12 — strapping; 25 — DAC; 21/22 — при отсутствии LoRa/nRF; всё board test. |
| GPIO25/26 не работают как digital | SSR/ENA — GPIO17 (если SP3232EEN не мешает) или GPIO16 после доработки | GPIO16 — выход SP3232EEN; нужна схема/доработка. |
| GPIO21/22 недоступны | STEP/DIR — GPIO16/17 (после доработки SP3232EEN) или 17 + 2 | GPIO2 — strapping; 16 — конфликт с SP3232EEN. |
| GPIO12 не проходит boot | UART — 13 + 17 (проверить SP3232EEN) или 13 + 26 | 25/26 как UART RX сомнительны из-за LM258DR; иначе — доработка/пересмотр. |
| Нужен ещё 1 выход | GPIO26, 16/17 или 2 (крайний вариант) | 2 — strapping; 16/17 — transceiver. |
| Нужен ENA дозирующего насоса | **Аппаратно/Nano cutoff**; опция GPIO26 | Не расходовать GPIO A6, если можно. |
| Нужны 3+ дополнительных выхода | Внешний PCF8574/PCF8575 на I2C **только для non-safety** сервиса | **Нельзя** выносить safety UART, STEP/DIR, safety-gate или cutoff на expander. **SSR pilot ТЭН — direct GPIO + pull-down, Hi-Z = выключено; на expander не выносить.** Это не аварийное отключение (BLK-002). |
| Нет свободного реле | Внешнее DIN-реле/контактор с драйвером | **Не параллелить** контакты; реле A6 не safety path. |
| Нужен ещё 220В-вход | Опто-вход `0x22` (non-safety) или внешний формирователь | Не использовать PCF8574 для safety-сигналов. |
| Больше DS18B20 | Одна/две шины 1-Wire, адресация | Проверить ёмкость/помехи/подтяжку. |
| Нужен ещё safety-вход | **Nano**, не A6 | Пересчёт бюджета Nano + `safety-reviewer`. |

**Главный вывод:** после отказа от RS232 и LoRa/nRF дефицит выходных GPIO снимается: 4 обязательных выхода покрываются 12 + 25 + 21 + 22, ENA — аппаратно/Nano. Но «чистых» линий только 21/22; 12 — strapping, 25/26 — DAC, 16/17 — SP3232EEN, 2 — strapping. Входной бюджет A6 перекрывает non-safety задачи. **Реле — 6/6, запаса нет.**

---

## 5. Конфликты и взаимоисключения

| Конфликт | Линии | Последствие | Решение |
|---|---|---|---|
| Extend serial ↔ safety UART | GPIO12/13 | Нельзя одновременно использовать extend serial и UART | Считать extend serial свободным **только** после board test. **GPIO12 — TX с pull-down**; как RX не использовать. |
| RS232 ↔ свободные GPIO | GPIO16/17 | RS232 не планируется, но SP3232EEN остаётся на плате | **GPIO16** — выход трансивера (не использовать как выход без доработки); **GPIO17** — вход (кандидат-выход, проверить нагрузку). |
| LoRa/nRF ↔ свободные GPIO | GPIO21/2/22 | Модули не планируются | **21/22 — лучшие чистые кандидаты** (STEP/DIR); **GPIO2 — strapping**. |
| DAC ↔ digital output | GPIO25/26 | LM258DR/аналоговый фронт может не пропустить цифровой уровень | Проверить измерением; при сомнении не использовать как GPIO. |
| 1-Wire ↔ свободные GPIO | GPIO32/33 | Отказ от шины ради выходов | Предпочтительно сохранить обе шины; объединение — только при дефиците. |
| SPI-сокет ↔ MAX6675 ↔ LoRa/nRF | GPIO5/18/19/23 + 21/2/22 | Один сокет — одна функция | P1: MAX6675; LoRa/nRF не устанавливать. GPIO5 — strapping. |
| STEP/DIR ↔ PCF8574/I2C | GPIO21/22 | I2C-expander не даёт детерминированных импульсов | **Только direct GPIO + hardware timer/RMT**. |
| Дозирующий насос ↔ safety | STEP/DIR, питание драйвера | A6 не safety path; залипание STEP/драйвера/помеха | **Nano cutoff питания драйвера**; BLK-002 scope; FMEA. |
| ADC/input-only ↔ выходы | GPIO34–39 | Физически не выходы | Только входы; аналоговый фронт учитывать. |
| I2C ↔ safety | GPIO4/15 | PCF8574 latch/reset/brownout | **Не safety path**; Nano hardware backup обязателен. |
| RS485 ↔ safety | GPIO14/27 | Без изоляции | **Не safety link**; только non-safety S-49. |
| USB/CH340 ↔ safety | GPIO1/3 | Debug-канал | **Не safety link**. |
| 6 реле ↔ 7+ нагрузок | PCF8574 `0x24` | Нет запаса | ТЭН — внешний SSR; дозирование — STEP/DIR; доп. нагрузки — внешние реле. |
| 220В/ЭМС ↔ сигнальные линии | Все 220В цепи, `SEN_FLAME`, UART | Помехи/ложные срабатывания | S-48: разводка, экран, snubber, clearance/creepage, ЭМС-тесты. |
| Strapping ↔ boot | GPIO0/2/5/12/15 | Boot fail/неверный режим | Проверять boot при каждом назначении; GPIO0 не использовать. |

---

## 6. Условия до утверждения (НЕ ВЫПОЛНЕНЫ)

- [ ] **BLK-012 / §7 board test:** ревизия v1.3, free GPIO 12/13/16/17/21/22/25/26, strapping, поведение реле/PCF8574 при POR/reset/brownout/power loss, ADC A1–A4, RS485, питание.
- [ ] **Зафиксировать отказ от RS232 и LoRa/nRF** (решение пользователя 2026-11-10) в `rnd/peripherals.md`/pinmap; иначе 16/17/21/2/22 **не считать свободными**.
- [ ] **SP3232EEN / LoRa-сокет:** проверить 16/17 (16 — выход трансивера, 17 — вход) и 21/2/22.
- [ ] **Дозирующий насос:** внести в `rnd/peripherals.md`; код и роль (замена/дополнение); **BLK-002 scope extension**; STEP/DIR/ENA, драйвер, питание, Nano cutoff, FMEA; `safety-reviewer`.
- [ ] **GPIO12 boot test:** проверить boot при low/high; зафиксировать допустимую схему.
- [ ] **GPIO25/26 digital test:** проверить цифровой выход при LM258DR; иначе исключить.
- [ ] **Relay POR test:** измерить состояние 6 реле до инициализации I2C, при reset/WDT/brownout; подтвердить NO/NC.
- [ ] **BLK-006:** формат протокола согласован с Nano до прошивки.
- [ ] **§7.18:** выбрать архитектуру изоляции/общего 0В; не смешивать барьеры; уровни 3.3 В ↔ 5 В.
- [ ] **BLK-005:** `SEN_FLAME` — схема, пороги, ЭМС PASS/FAIL, fallback; проверка до реализации.
- [ ] **BLK-002:** общий Nano cutoff, SR-B4 теплушка/термопредохранители, feedback, weld FMEA.
- [ ] **BLK-003:** граница MAX6675 зафиксирована (технологическая обратная связь, не Nano-interlock).
- [ ] **BLK-007:** схема `ACT_PUMP_BOILER`.
- [ ] **BLK-001/SR-B6:** board-off A6, поведение вентилятора, power-cycle.
- [ ] **S-49/BLK-010:** RS485 сегменты, термирование, slave fail-safe.
- [ ] **S-48:** 220В монтаж, изоляция, УЗО, предохранители, транзиенты.
- [ ] **Approvals:** `schematic-engineer` + `safety-reviewer`; после — фиксация в `docs/hardware/pinmap.md`.

---

## 7. Board test, специфичный для этого плана

1. **GPIO12/13:** extend serial не используется; GPIO12 — TX с pull-down, GPIO13 — RX; boot при подключённом Nano (Nano TX idle high не должен попадать на GPIO12). Измерить уровни/фронты, поведение при board-off A6, процедуру прошивки Nano при подключённом safety UART (D0/D1 vs CH340).
2. **RS232 16/17:** SP3232EEN не мешает; GPIO16 как выход — только после доработки; GPIO17 — измерить нагрузку/фронты.
3. **LoRa/nRF 21/2/22:** сокет не установлен; 21/22 — проверить как STEP/DIR; 2 — boot/strapping.
4. **GPIO25/26:** проверить цифровой выход при LM258DR; уровни/фронты.
5. **Реле:** до инициализации I2C, при reset, WDT, brownout, power loss — состояние контактов; NO/NC.
6. **PCF8574:** I2C-адреса `0x22/0x24`, пропадание I2C, latch.
7. **ADC A1/A2:** диапазон/точность/импеданс; A3/A4 — 0–5 В vs 4–20 мА.
8. **SPI-сокет:** MAX6675 CS/SCK/MISO/MOSI; GPIO5 boot; отсутствие конфликта с LoRa/nRF.
9. **1-Wire:** обе шины, подтяжка, количество DS18B20, помехи.
10. **RS485:** терминирование/bias/TVS, DE/RE, отсутствие изоляции.
11. **Питание:** ток с реле/Wi-Fi, бросок при board-off/on, тепловой режим.
12. **Совместимость с Nano:** safety-функции Nano работают при reset/I2C fault/board-off/latch A6.
13. **Дозирующий насос:** STEP/DIR на 21/22, максимальная частота/джиттер, ENA/драйвер, обесточивание драйвера Nano cutoff; отсутствие подачи при остановке импульсов/снятии питания и ложных шагов при reset/boot/brownout A6 и ВВ-розжиге (совместно с BLK-005).

---

## 8. Итоговая рекомендация

1. **P1 — базовый сценарий для board test (не approval; все линии CANDIDATE):** 6 onboard реле — под 6 прикладных нагрузок; ТЭН — внешний SSR (GPIO25); safety UART — **GPIO12 TX / GPIO13 RX**; дозирующий насос — **STEP GPIO21 / DIR GPIO22**, ENA — аппаратно/Nano (опция GPIO26); flame — A1/A2; MAX6675 — SPI; DS18B20 — 32/33. Резерв — **16/17, 2, 26**.
2. **Зафиксировать отказ от RS232 и LoRa/nRF:** освобождает 16/17 и 21/2/22, но решение должно быть внесено в документы и подтверждено board test.
3. **Дозирующий насос — новый топливный актуатор:** A6 даёт только STEP/DIR (direct GPIO, hardware timer); **питание драйвера — через Nano cutoff**, не через реле A6/PCF8574. Требуется **BLK-002 scope extension**, обновление `rnd/peripherals.md`, FMEA и `safety-reviewer`.
4. **Не считать это pinmap:** до board test и approvals все линии — CANDIDATE/TBD; подтверждённо свободных выходов **0**.
5. **Реле — 6/6, запаса нет:** дополнительная 220В-нагрузка — только внешнее DIN-реле/контактор; параллелить контакты нельзя.
6. **Safety-инварианты не трогать:** safety-gates, `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, поплавки, общий cutoff, SR-B6 — только Nano.
7. **Следующий шаг:** board test §7 → уточнение free GPIO/реле/SP3232EEN → `schematic-engineer` → `safety-reviewer` → фиксация в `pinmap.md`.

---

## 9. Связанные документы

- `docs/hardware/kc868-a6_analysis.md` — факты A6, §6 условия, §7 board test, BLK-012.
- `docs/hardware/pinmap.md` — пин-мап (DRAFT); окончательная фиксация — там.
- `docs/hardware/periph_distribution.md` — распределение Main/Nano.
- `docs/architecture/blockers.md` — BLK-001…BLK-012, gates.
- `docs/architecture/modules.md` — границы ответственности.
- `docs/protocol/uart.md` — safety UART, BLK-006.
- `docs/safety/checklist.md` — safety-инварианты и board verification.
- `rnd/peripherals.md` — источник правды по периферии.

## 10. Внешние источники A6

- <https://www.kincony.com/esp32-6-channel-relay-module.html>
- <https://www.kincony.com/download/KC868-A6-schematic.pdf>
- <https://www.kincony.com/forum/showthread.php?tid=1962>
- <https://www.kincony.com/forum/showthread.php?tid=1963>

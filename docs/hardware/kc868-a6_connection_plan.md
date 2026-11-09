# План подключений KC868-A6 (DRAFT / NOT APPROVED)

> **СТАТУС: ПРЕДВАРИТЕЛЬНЫЙ ПЛАН. НЕ УТВЕРЖДЕНО. НЕ ЯВЛЯЕТСЯ PINMAP/SCHEMATIC APPROVAL.**
>
> Документ отвечает на вопрос: **к каким линиям KC868-A6 (не v3) предварительно планируется подключить периферию проекта, какие линии свободны, каких линий не хватает.**
>
> Плата: **KC868-A6, ревизия v1.3**, ESP32-WROOM-32, USB-C + CH340C. Дата: 2026-11-10.
>
> **Границы документа:**
> - это **не** замена `docs/hardware/pinmap.md`; окончательные назначения фиксируются только там;
> - это **не** схема подключения и **не** основание для монтажа/прошивки;
> - все назначения ниже — **CANDIDATE/TBD**, пока не выполнены board test (`docs/hardware/kc868-a6_analysis.md` §7), не закрыт **BLK-012** и нет approvals `schematic-engineer` + `safety-reviewer`;
> - Safety Unit **Arduino Nano не заменяется**; его инварианты и финальное право блокировки сохраняются.
>
> **Ключевой вывод:** входов у A6 достаточно, **реле — ровно 6/6 без запаса**, а **свободных выходных GPIO практически нет**. Каждый дополнительный выход — это отказ от штатной функции платы (extend serial / RS232 / DAC / 1-Wire / SPI-сокет). Поэтому план ниже — это бюджет линий, а не утверждённая разводка.

---

## Легенда статусов

| Статус | Значение |
|---|---|
| **FIXED-DRAFT** | Роль/инвариант зафиксирован архитектурно; физическая линия/клемма не утверждена. |
| **CANDIDATE** | Предлагаемая линия; требует board test/схемы/ревью. |
| **TBD** | Уточнение обязательно; не финальное. |
| **BLOCKED** | Нельзя финализировать/реализовывать до закрытия блокера/approval. |

---

## 1. Инвентаризация линий A6

### 1.1. GPIO ESP32-WROOM-32 на плате A6

| GPIO | Штатная функция A6 / источник | Тип | По умолчанию | Возможное использование в проекте | Статус |
|---|---|---|---|---|---|
| 0 | Boot strapping | Strapping | Boot | **Не использовать** | — |
| 1 | UART0 TX → CH340C | Debug UART | Занят | Только прошивка/лог | **FIXED-DRAFT** (занят) |
| 2 | LoRa DIO0 | Сокет + **strapping** | Занят сокетом | Только при отказе от сокета; boot-риск | **TBD** |
| 3 | UART0 RX ← CH340C | Debug UART | Занят | Только прошивка/лог | **FIXED-DRAFT** (занят) |
| 4 | I2C SDA | I2C | Занят | PCF8574 `0x22/0x24`, сервис | **FIXED-DRAFT** (занят) |
| 5 | SPI CS | Сокет + **strapping** | Занят сокетом | MAX6675 CS / только при отказе от сокета | **TBD** |
| 6–11 | Internal SPI flash | Недоступны | Заняты | **Не использовать** | — |
| 12 | Extend serial TXD | UART + **strapping (MTDI)** | Условно свободен | **Кандидат №1 под safety UART**; проверить boot | **CANDIDATE (board test)** |
| 13 | Extend serial RXD | UART (MTCK/JTAG) | Условно свободен | **Кандидат №1 под safety UART** | **CANDIDATE (board test)** |
| 14 | RS485 RXD | RS485 | Занят | Non-safety RS485 | **FIXED-DRAFT** (занят) |
| 15 | I2C SCL | I2C + **strapping (MTDO)** | Занят | PCF8574, сервис | **FIXED-DRAFT** (занят) |
| 16 | RS232 RXD | RS232 | Занят, если RS232 нужен | Резерв при отказе от RS232 | **TBD** |
| 17 | RS232 TXD | RS232 | Занят, если RS232 нужен | Резерв при отказе от RS232 | **TBD** |
| 18 | SPI SCK | Сокет | Занят сокетом | MAX6675 SCK | **TBD** |
| 19 | SPI MISO | Сокет | Занят сокетом | MAX6675 MISO | **TBD** |
| 21 | LoRa RST | Сокет | Занят сокетом | Резерв при отказе от сокета | **TBD** |
| 22 | nRF CE | Сокет | Занят сокетом | Резерв при отказе от сокета | **TBD** |
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

**Недоступны на ESP32-WROOM-32:** GPIO20, 24, 28–31, 37–38 (нет на модуле/заняты внутренними цепями).  
**Strapping-пины A6:** GPIO0, 2, 5, 12, 15 — любое назначение только после проверки boot.  
**Input-only:** GPIO34–39 — **не могут** управлять SSR, реле, UART TX; как вход использовать можно, но на A6 к ним подключён аналоговый фронт (делители/ОУ), что нужно учитывать.

### 1.2. Встроенные ресурсы платы (не GPIO, но входят в бюджет)

| Ресурс A6 | Линия/адрес | Кол-во | Предварительное использование | Статус |
|---|---|---:|---|---|
| PCF8574 «реле» | I2C `0x24`, ch0–ch5, inverted | 6 | Прикладное управление 6 нагрузками | **CANDIDATE** (не safety path) |
| Реле SPDT + ULN2003A | Клеммы PCOM/NC/NO | 6 | См. §2.1 | **CANDIDATE** (POR/reset — board test) |
| PCF8574 «входы» | I2C `0x22`, ch0–ch5, inverted | 6 | Технологические входы (220В presence, сервис) | **TBD** (не safety path) |
| Опто-входы EL357N | 12 В, через `0x22` | 6 | Non-safety дискретные сигналы | **TBD** |
| ADC A1/A2 | GPIO36/39, делители 0–5 В | 2 | `SEN_FLAME` (BLK-005) | **BLOCKED (BLK-005)** |
| ADC A3/A4 | GPIO34/35, ОУ/BAT54S | 2 | Резерв; режим TBD | **TBD** |
| 1-Wire | GPIO32, GPIO33 | 2 шины | Технологические DS18B20 (до ~12 датчиков) | **CANDIDATE** |
| I2C | GPIO4/15 | 1 шина | PCF8574 + сервис | **FIXED-DRAFT** |
| RS485 | GPIO14/27, MAX13487EESA, **без изоляции** | 1 | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` (non-safety) | **CANDIDATE (S-49/BLK-010)** |
| RS232 | GPIO16/17, SP3232EEN | 1 | Резерв/сервис/HMI | **TBD** |
| SPI-сокет | GPIO5/18/19/23 + 21/2/22 | 1 | MAX6675 (`SEN_THERMOCOUPLE_K`) **или** LoRa/nRF | **CANDIDATE (BLK-003 ACK)** |
| DAC | GPIO25/26 + LM258DR | 2 | SSR pilot / 0–10 В привод (non-safety) | **CANDIDATE** |
| USB-C + CH340C | UART0 GPIO1/3 | 1 | Прошивка/отладка/лог | **FIXED-DRAFT** (не safety link) |
| Питание | 12 В → XL1509-5.0 → LM1117-3.3 | — | Питание платы; SR-B6 board-off | **TBD (board test)** |

### 1.3. Сводка: что реально свободно

| Категория | Линии | Комментарий |
|---|---|---|
| **Подтверждённо свободные** | **нет** | Официального списка свободных GPIO у A6 нет. |
| **Условно свободные (отказ от extend serial)** | **GPIO12, GPIO13** | GPIO12 — strapping (MTDI): boot-риск. Это лучшие кандидаты под safety UART, но **только после board test**. |
| **Свободны только при отказе от функции** | GPIO16/17 (RS232), GPIO25/26 (DAC), GPIO32/33 (1-Wire), GPIO21/2/22 (LoRa/nRF) | Каждая линия — отказ от штатного интерфейса. GPIO2 — strapping. |
| **Только входы** | GPIO34, 35, 36, 39 | ADC/input-only; не выходы. На A6 имеют аналоговый фронт. |
| **Недоступны/заняты критично** | GPIO0/1/3 (boot/UART0/CH340), GPIO6–11 (flash), GPIO4/15 (I2C), GPIO14/27 (RS485), GPIO5/18/19/23 (SPI-сокет) | Не расходовать без пересмотра архитектуры. |

> **Практический вывод:** при сохранении всех штатных функций A6 у проекта **0 подтверждённых свободных выходных GPIO**. Реально доступны только **12/13** (extend serial), и то с оговоркой по GPIO12. Всё остальное — «свободно, если отказаться».
>
> ⚠️ **Критично для safety UART:** GPIO12 — strapping (MTDI) и требует **pull-down** на boot; использовать его как UART **RX** нельзя без отдельного анализа (Nano TX idle high может сорвать загрузку ESP32). Рабочий вариант — **GPIO12 = TX с pull-down, GPIO13 = RX**. Если board test не подтвердит GPIO12 даже в режиме TX, **чистой пары GPIO под safety UART на A6 нет** — это блокер, требующий схемной доработки или пересмотра платы.

---

## 2. Бюджет линий и дефицит

### 2.1. Бюджет реле: 6 нужно / 6 есть / 0 запаса

| № | Нагрузка (код) | Нужно реле A6 | Контакт (предпочтительно) | Примечание |
|---:|---|---:|---|---|
| 1 | `ACT_FAN_BLOWER` | 1 | NO | BLK-001: per-circuit Nano cutoff не требуется; SR-B6 coarse board-off. |
| 2 | `ACT_OIL_VALVE_MAIN` | 1 | NO | Последовательно с `ACT_OIL_VALVE_SAFETY` (NC, только Nano). |
| 3 | `ACT_AIR_VALVE_MAIN` | 1 | NO | Последовательно с `ACT_AIR_VALVE_SAFETY` (NC, только Nano). |
| 4 | `ACT_IGNITION_UNIT` | 1 | NO | В группе общего Nano cutoff (BLK-002). |
| 5 | `ACT_PUMP_OIL_TRANSFER` | 1 | NO | Nano hardware block/stop OFF (BLK-002/SR-B18). |
| 6 | `ACT_PUMP_BOILER` | 1 | NO/NC — TBD по BLK-007 | Политика «оставание ON» / выбег — решается схемой. |
| — | **Итого onboard реле** | **6/6** | — | **Свободных реле нет.** |
| 7 | `ACT_HEATER_OIL` | **0** | Внешний SSR/драйвер | Не onboard relay как силовой путь; Nano series cutoff + теплушка. |
| — | `ACT_OIL_VALVE_SAFETY`, `ACT_AIR_VALVE_SAFETY` | **0** | Только Nano | На A6 не подключаются. |
| — | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` | **0** | RS485 slave | Non-safety, S-49. |

**Вывод:** 6 реле A6 покрывают ровно 6 прикладных нагрузок. **Седьмая нагрузка (`ACT_HEATER_OIL`) выносится на внешний SSR/драйвер.** Любая дополнительная 220В-нагрузка потребует внешнего DIN-реле/контактора; **параллелить контакты реле нельзя**, использовать реле A6 как safety path нельзя.

> ⚠️ **NO — это схемный выбор «обесточен = нагрузка выключена», а не гарантия fail-safe.** Поведение PCF8574/ULN2003A/реле при POR/reset/brownout/power loss **не подтверждено**; до board test нельзя считать, что реле A6 гарантированно разомкнуто. Safety-функции остаются на Nano.

### 2.2. Бюджет direct GPIO

| Функция | Направление | Нужно линий | Кандидат A6 | Подтверждено? |
|---|---|---:|---|---|
| Safety UART TX (KC868 → Nano) | Выход | 1 | **GPIO12** (extend serial TXD) + внешний pull-down | **Нет** |
| Safety UART RX (Nano → KC868) | Вход | 1 | **GPIO13** (extend serial RXD) | **Нет** |
| `ACT_HEATER_OIL` SSR pilot | Выход | 1 | GPIO25 / GPIO26 (DAC) | **Нет** |
| `SEN_FLAME` fallback digital/Schmitt | Вход | 0–1 | GPIO39 (A2) как digital input | **Нет (BLK-005)** |
| Детект пропадания 220В | Вход | 0–1 | Опто-вход `0x22` (не GPIO) или ADC | **Нет** |
| Сервис/индикация/диагностика | Выход | 0–1 | Свободных нет; использовать USB/лог | **Нет** |
| **Итого обязательных выходов** | — | **2** (UART TX + SSR) | 12/13 + 25/26 | **0 подтверждено** |
| **Итого обязательных входов** | — | **1** (UART RX) + опционально flame/220В | 12/13, 39, опто | **0 подтверждено** |

**Расчёт дефицита:**

- При сохранении **всех** штатных функций A6: свободных выходов **0** → дефицит **2** (UART TX + SSR).
- При отказе от extend serial: доступны **12/13** → покрывают UART TX/RX. Остаётся **1 выход** (SSR) — берём **25 или 26** (отказ от одного канала DAC).
- Если **GPIO12 не проходит boot-тест** (в т.ч. с внешним pull-down): чистых линий под UART TX + SSR остаётся 2 (GPIO13 + один из 25/26) при потребности 3 → **дефицит 1**. Второй канал DAC / 16/17 / 32/33 — только с отказом от функции и проверкой, что фронт не мешает UART. Если и это невозможно — **схемная доработка или пересмотр платы**.
- Если нужен **сервисный выход** (LED/реле аварии/вентилятор шкафа): дефицит **+1**.
- **Входы:** 4 ADC/input-only + 6 опто-входов — для non-safety задач достаточно. Дефицита входов нет.

### 2.3. Итоговая оценка

| Ресурс | Оценка |
|---|---|
| Реле | **6/6, запаса нет**; ТЭН — внешний SSR. |
| Direct outputs | **Критически мало**; 2 обязательных, доступно 1–2 условно. |
| Direct inputs | Достаточно (ADC/input-only + опто). |
| I2C | Занята PCF8574; расширяема, но **не safety path**. |
| RS485 | Есть, но **без изоляции** и только non-safety. |
| Safety UART | Линий нет «бесплатных»; требует отказа от extend serial и board test. |

---

## 3. Предварительный план P1 (baseline)

> Ниже — **согласованный по смыслу, но не утверждённый** вариант. Он минимизирует отказ от функций: использует extend serial под safety UART, один канал DAC под SSR, SPI-сокет под MAX6675, обе шины 1-Wire под DS18B20.

### 3.1. Реле A6 (PCF8574 `0x24`)

| Канал | Линия | Нагрузка | Контакт | Статус | Условие/блокер |
|---:|---|---|---|---|---|
| ch0 | Relay #1 | `ACT_FAN_BLOWER` | NO | **CANDIDATE** | BLK-001; POR/reset board test; SR-B6. |
| ch1 | Relay #2 | `ACT_OIL_VALVE_MAIN` | NO | **CANDIDATE** | Safety-gate Nano последовательно; 220В snubber. |
| ch2 | Relay #3 | `ACT_AIR_VALVE_MAIN` | NO | **CANDIDATE** | Safety-gate Nano последовательно. |
| ch3 | Relay #4 | `ACT_IGNITION_UNIT` | NO | **CANDIDATE** | BLK-002; ЭМС ВВ розжига. |
| ch4 | Relay #5 | `ACT_PUMP_OIL_TRANSFER` | NO | **CANDIDATE** | BLK-002/SR-B18. |
| ch5 | Relay #6 | `ACT_PUMP_BOILER` | NO/NC TBD | **CANDIDATE** | BLK-007; обратное питание/изоляция. |

### 3.2. Direct GPIO и аналоговые линии

| № | Функция | Линия A6 | Режим | Статус | Условие/блокер |
|---:|---|---|---|---|---|
| D1 | Safety UART TX → Nano | **GPIO12** (extend serial TXD) | UART TX + внешний **pull-down** | **CANDIDATE** | **GPIO12 strapping (MTDI):** pull-down удерживает low на boot; после boot — TX idle high. Boot test обязателен; BLK-006. |
| D2 | Safety UART RX ← Nano | **GPIO13** (extend serial RXD) | UART RX | **CANDIDATE** | GPIO13 не strapping; Nano TX idle high допустим. BLK-006; §7.18. |
| D3 | `ACT_HEATER_OIL` SSR pilot | **GPIO25** (DAC2) | Digital out / альтернатива GPIO26 | **CANDIDATE** | Проверить, что LM258DR не мешает цифровому уровню; pull-down; BLK-002. |
| D4 | `SEN_FLAME` основной | **ADC A1 = GPIO36** | Analog 0–5 В | **BLOCKED (BLK-005)** | Только после APPROVE; экран/ЭМС; обрыв/КЗ = «нет пламени». |
| D5 | `SEN_FLAME` fallback | **ADC A2 = GPIO39** | Digital input (изолированный формирователь) | **BLOCKED (BLK-005)** | Input-only; использовать как вход, не выход. Учесть делитель A2 (0–5 В) и импеданс формирователя. |
| D6 | `SEN_THERMOCOUPLE_K` + MAX6675 | **SPI: CS5 / SCK18 / MISO19 / MOSI23** | SPI | **CANDIDATE** | BLK-003 ACK; GPIO5 strapping; сокет не занимать LoRa/nRF. |
| D7 | Технологические DS18B20 | **1-Wire 1 = GPIO32**, **1-Wire 2 = GPIO33** | 1-Wire | **CANDIDATE** | Подтяжка/экран/разводка 12 датчиков; non-safety. |
| D8 | Детект 220В | **Опто-вход `0x22` ch0** (или ADC A3/A4 после подтверждения) | Discrete 12 В | **TBD** | Внешний 220В→12В формирователь; non-safety. |
| D9 | Сервис/индикация | **Не выделять** | — | **TBD** | Свободных выходов нет; использовать USB/CH340/лог. |

### 3.3. Что НЕ подключается к A6 (инварианты Nano)

| Функция | Куда | Почему |
|---|---|---|
| `ACT_OIL_VALVE_SAFETY` (NC) | **Только Nano (D5)** | Независимый safety-gate; Main Unit не управляет. |
| `ACT_AIR_VALVE_SAFETY` (NC) | **Только Nano (D6)** | Независимый safety-gate. |
| `SEN_AIR_PRESSURE` | **Прямой Nano (D2)** | Критичный датчик; не через PCF8574/I2C. |
| `SEN_OIL_TEMP_SAFETY` (DS18B20) | **Прямой Nano (D11)** | Независимая аварийная отсечка. |
| `SEN_FLOAT_MIN` | **Прямой Nano (D3)** | Критический минимум. |
| `SEN_FLOAT_MAX` | **Прямой Nano (D4)** | Отсечка подкачки. |
| Общий Nano cutoff (ТЭН/подкачка/розжиг) | **Nano (D7/D8)** | BLK-002; hardware, no bypass. |
| SR-B6 power-cycle 12В A6 | **Nano (D9/D10)** | Только Nano; A6 не управляет своим отключением. |

### 3.4. Non-safety интерфейсы

| Интерфейс | Линия | Использование | Статус |
|---|---|---|---|
| RS485 | GPIO14/27 | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` (slave), S-49 | **CANDIDATE** |
| RS232 | GPIO16/17 | Резерв/сервис/HMI; **не safety** | **TBD** |
| I2C | GPIO4/15 | PCF8574 `0x22/0x24`, сервис | **FIXED-DRAFT** |
| USB-C/CH340C | UART0 | Прошивка/отладка/лог; **не safety link** | **FIXED-DRAFT** |
| LoRa/nRF сокет | GPIO21/2/22 + SPI | **Не использовать** в P1 | **TBD** |
| DAC второй канал | GPIO26 | Резерв (0–10 В привод/диагностика) | **TBD** |

---

## 4. Нехватка линий: сценарии и митигации

| Сценарий | Что не хватает | Митигация | Ограничение |
|---|---|---|---|
| Нужен ещё 1 выход | 1 GPIO | Отказаться от второго канала DAC (GPIO26) **или** объединить все DS18B20 на одну шину и освободить GPIO33 | Только после board test; safety-функции не переносить на expander. |
| Нужны ещё 2 выхода | 2 GPIO | Отказаться от RS232 (GPIO16/17) **или** от второго канала DAC + одной шины 1-Wire | RS232/HMI теряется; 1-Wire разводка усложняется. |
| Нужны 3+ выхода | 3+ GPIO | Внешний PCF8574/PCF8575 на I2C **только для non-safety** сервиса; сократить функции | **Нельзя** выносить safety UART, safety-gate, cutoff, SSR safety pilot на expander. |
| GPIO12 не проходит boot | 1 выход | UART: GPIO13 + один из 25/26; SSR — второй 25/26 или 16/17/32/33 | Проверить, что 25/26 пригодны как вход/выход; иначе — схемная доработка/пересмотр. |
| Нет свободного реле | 1+ реле | Внешнее DIN-реле/контактор с драйвером | **Не параллелить** контакты; реле A6 не safety path. |
| Нужен ещё 220В-вход | 1 вход | Опто-вход `0x22` (non-safety) или внешний формирователь | Не использовать PCF8574 для safety-сигналов. |
| Больше DS18B20 | 0 GPIO | Одна/две шины 1-Wire, адресация | Проверить ёмкость/помехи/подтяжку. |
| Нужен ещё safety-вход | 1 вход | **Nano**, не A6 | Пересчёт бюджета Nano + `safety-reviewer`. |

**Главный дефицит — не входы, а выходные GPIO и реле.** Входной бюджет A6 (4 ADC/input-only + 6 опто) перекрывает non-safety задачи. Выходной бюджет собирается только за счёт отказа от штатных функций.

---

## 5. Конфликты и взаимоисключения

| Конфликт | Линии | Последствие | Решение |
|---|---|---|---|
| Extend serial ↔ safety UART | GPIO12/13 | Нельзя одновременно использовать extend serial и UART | Считать extend serial свободным ресурсом **только** после board test. **GPIO12 — TX с pull-down** (strapping MTDI); использовать GPIO12 как RX нельзя без анализа: Nano TX idle high может сорвать boot. |
| RS232 ↔ свободные GPIO | GPIO16/17 | Отказ от RS232 ради выходов | Если RS232 не нужен — резерв; иначе не трогать. |
| DAC ↔ digital output | GPIO25/26 | LM258DR/аналоговый фронт может не пропустить цифровой уровень | Проверить измерением; при сомнении — не использовать как GPIO. |
| 1-Wire ↔ свободные GPIO | GPIO32/33 | Отказ от шины ради выходов | Предпочтительно сохранить обе шины; объединение — только при дефиците. |
| SPI-сокет ↔ MAX6675 ↔ LoRa/nRF | GPIO5/18/19/23 + 21/2/22 | Один сокет — одна функция | P1: MAX6675; LoRa/nRF не устанавливать. GPIO5 — strapping. |
| ADC/input-only ↔ выходы | GPIO34–39 | Физически не выходы | Использовать только как входы; аналоговый фронт учитывать. |
| I2C ↔ safety | GPIO4/15 | PCF8574 latch/reset/brownout | **Не safety path**; Nano hardware backup обязателен. |
| RS485 ↔ safety | GPIO14/27 | Без изоляции | **Не safety link**; только non-safety S-49. |
| USB/CH340 ↔ safety | GPIO1/3 | Debug-канал | **Не safety link**; safety UART — отдельный. |
| 6 реле ↔ 7+ нагрузок | PCF8574 `0x24` | Нет запаса | ТЭН — внешний SSR; доп. нагрузки — внешние реле. |
| 220В/ЭМС ↔ сигнальные линии | Все 220В цепи, `SEN_FLAME`, UART | Помехи/ложные срабатывания | S-48: разводка, экран, snubber, clearance/creepage, ЭМС-тесты. |
| Strapping ↔ boot | GPIO0/2/5/12/15 | Boot fail/неверный режим | Проверять boot при каждом назначении; GPIO0 не использовать. |

---

## 6. Условия до утверждения (НЕ ВЫПОЛНЕНЫ)

- [ ] **BLK-012 / §7 board test:** подтвердить ревизию v1.3, free GPIO 12/13, strapping, поведение реле/PCF8574 при POR/reset/brownout/power loss, ADC A1–A4, RS485, питание.
- [ ] **GPIO12 boot test:** проверить boot при low/high на GPIO12; зафиксировать допустимую схему подключения.
- [ ] **GPIO25/26 digital test:** проверить, работает ли цифровой выход при подключённом LM258DR; иначе исключить из плана.
- [ ] **Relay POR test:** измерить состояние всех 6 реле до инициализации I2C, при reset/WDT/brownout; подтвердить выбранные NO/NC клеммы.
- [ ] **BLK-006:** `safety-reviewer` APPROVE протокола safety UART до прошивки.
- [ ] **§7.18:** выбрать архитектуру изоляции/общего 0В; не смешивать барьеры; уровни 3.3 В ↔ 5 В.
- [ ] **BLK-005:** `SEN_FLAME` — схема, пороги, ЭМС PASS/FAIL, fallback; APPROVE до реализации.
- [ ] **BLK-002:** общий Nano cutoff, SR-B4 теплушка/термопредохранители, feedback, weld FMEA/ACK.
- [ ] **BLK-003:** `safety-reviewer` ACK границы MAX6675 (не safety-зона Nano).
- [ ] **BLK-007:** схема `ACT_PUMP_BOILER` (Nano enable/приоритеты/обратное питание).
- [ ] **BLK-001/SR-B6:** board-off A6, поведение вентилятора, power-cycle.
- [ ] **S-49/BLK-010:** RS485 сегменты, термирование, slave fail-safe.
- [ ] **S-48:** 220В монтаж, изоляция, УЗО, предохранители, транзиенты.
- [ ] **Approvals:** `schematic-engineer` + `safety-reviewer`; после — фиксация в `docs/hardware/pinmap.md`.

---

## 7. Board test, специфичный для этого плана

1. **GPIO12/13:** проверить, что extend serial не используется; GPIO12 — TX с pull-down, GPIO13 — RX; проверить boot при подключённом Nano (Nano TX idle high не должен попадать на GPIO12).
2. **GPIO25/26:** подключить нагрузку/осциллограф; проверить цифровой выход при наличии LM258DR; измерить уровни/фронты.
3. **Реле:** до инициализации I2C, при reset, WDT, brownout, power loss — состояние контактов; проверить NO/NC.
4. **PCF8574:** I2C-адреса `0x22/0x24`, поведение при пропадании I2C, latch.
5. **ADC A1/A2:** диапазон/точность/импеданс; A3/A4 — 0–5 В vs 4–20 мА.
6. **SPI-сокет:** MAX6675 CS/SCK/MISO/MOSI; GPIO5 boot; отсутствие конфликта с LoRa/nRF.
7. **1-Wire:** обе шины, подтяжка, количество DS18B20, помехи.
8. **RS485:** термирование/bias/TVS, DE/RE, отсутствие изоляции.
9. **Питание:** ток с реле/Wi-Fi, бросок при board-off/on, тепловой режим.
10. **Совместимость с Nano:** safety-функции Nano работают при reset/I2C fault/board-off/latch A6.

---

## 8. Итоговая рекомендация

1. **Принять P1 как рабочий черновик:** 6 onboard реле — под 6 прикладных нагрузок; ТЭН — внешний SSR; safety UART — 12/13; SSR pilot — 25/26; flame — A1/A2; MAX6675 — SPI; DS18B20 — 32/33.
2. **Не считать это pinmap:** до board test и approvals все линии — CANDIDATE/TBD.
3. **Заранее заложить дефицит:** 0 свободных реле и 0 подтверждённых свободных выходных GPIO; любой дополнительный выход — отказ от функции или внешний expander (non-safety).
4. **Safety-инварианты не трогать:** safety-gates, `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, поплавки, общий cutoff, SR-B6 — только Nano.
5. **Следующий шаг:** board test §7 → уточнение free GPIO/реле → `schematic-engineer` → `safety-reviewer` → фиксация в `pinmap.md`.

---

## 9. Связанные документы

- `docs/hardware/kc868-a6_analysis.md` — подтверждённые факты A6, §6 условия, §7 board test, BLK-012.
- `docs/hardware/pinmap.md` — пин-мап (DRAFT); окончательная фиксация — там.
- `docs/hardware/io_analysis.md` — достаточность I/O.
- `docs/hardware/periph_distribution.md` — распределение Main/Nano.
- `docs/architecture/blockers.md` — BLK-001…BLK-012, gates.
- `docs/architecture/modules.md` — границы ответственности.
- `docs/protocol/uart.md` — safety UART, BLK-006.
- `docs/safety/checklist.md` — safety-инварианты и board verification.
- `rnd/peripherals.md` — источник правды по периферии.
- `docs/hardware/kc868-a6v3_analysis.md` — архив/сравнение; v3-данные к A6 **не применять**.

## 10. Внешние источники A6

- <https://www.kincony.com/esp32-6-channel-relay-module.html>
- <https://www.kincony.com/download/KC868-A6-schematic.pdf>
- <https://www.kincony.com/forum/showthread.php?tid=1962>
- <https://www.kincony.com/forum/showthread.php?tid=1963>

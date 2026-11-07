# Анализ достаточности входов/выходов и выбор основного контроллера

Статус: **актуализирован** (шаг 3, предварительная проверка ресурсов; архитектурное решение — Main Unit = **KC868-A6v3**, safety-gated по `docs/hardware/kc868-a6v3_analysis.md` §7/§14).  
Основано на: `docs/architecture/modules.md`, `docs/hardware/periph_distribution.md`, `rnd/peripherals.md`, `docs/hardware/kc868-a6v3_analysis.md`.

## 1. Требования к I/O (по распределению периферии)

### 1.1. Main Unit — KC868-A6v3 (ESP32-S3-WROOM-1 N16R8)

#### Подтверждённые ресурсы платы (кандидат, не пин-мап)
| Ресурс | Кол-во / интерфейс | Примечание (safety/архитектура) |
|---|---:|---|
| Onboard реле | 6, 250В/10А (пилотные) | Через PCF8574 `0x24` по общей I2C. **Не независимый/fail-safe safety path** (latch/reset/brownout/3.3V sag/I2C fault). Допустимы **только для прикладного управления** и **только при обязательных независимых Nano hardware safety-gate/cutoff/block-stop в силовых цепях** (Nano приоритет/финальное право блокировки; не программный fallback). |
| Опто-входы | 6 | Через PCF8574 `0x22` по общей I2C. **Только технологические/не safety-критичные сигналы.** Критичные датчики Nano (`SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_MIN/MAX`) — прямые на Nano. |
| Аналоговые входы | 4 (0–5В) | `SEN_FLAME` (BLK-005, PROPOSED/NOT VALIDATED): решение пользователя — фоторезистор на аналоговый вход KC868 + экранирование + ЭМС-тесты при розжиге; fallback opto/изолированный формирователь (digital/Schmitt) на direct GPIO. Отклонение от SR-B13/S-47; требуется `safety-reviewer`+`schematic-engineer` APPROVE. |
| 1-Wire | 2 (free terminals) | Non-safety. S-49 runtime limits/fallback. |
| RS485 | 1 (UART/трансивер на плате, детали изоляции/терминирования — §11) | Non-safety доп.устройства. S-49: отдельные сегменты при DMX/Modbus. |
| RS232 | 1 | Взаимоисключения/назначение — pinmap. |
| USB | Native USB-C (ESP32-S3) | Debug/прошивка. |
| RTC / OLED | DS3231 / OLED | На общей I2C; сервисные, не safety. |
| Свободные GPIO | 6 подтверждённых (внешние анализы; точный список — §7.16/§11) | Кандидаты для direct GPIO (`SEN_FLAME` формирователь), safety UART к Nano, SSR control и др. **Пины не фиксируются до pinmap.** |

#### Прикладные актуаторы Main Unit (через внешние модули, не safety path)
| Элемент | Прямой GPIO KC868 | Внешний модуль / примечание |
|---|---:|---|
| `ACT_IGNITION_UNIT` | 0 (через внешний модуль) | Команда от Main Unit; Nano — независимая блокировка. Механическая коммутация по схеме. |
| `ACT_OIL_VALVE_MAIN` | 0 | Внешний модуль; safety-gate Nano последовательно. |
| `ACT_AIR_VALVE_MAIN` | 0 | Внешний модуль; safety-gate Nano последовательно. |
| `ACT_FAN_BLOWER` | 0 | Внешний модуль. |
| `ACT_HEATER_OIL` | 0–1 (управление SSR/драйвером, кандидат direct GPIO) | **Внешний SSR/драйвер + Nano series cutoff в силовой цепи последовательно с ТЭН (SR-B4) + термостат/предохранители.** SSR не аварийное отключение. |
| `ACT_PUMP_OIL_TRANSFER` | 0 | Прикладное управление внешним модулем/FPC (опция). **Nano hardware block/stop в силовой цепи, fail-safe OFF (SR-B18).** |
| `ACT_PUMP_BOILER` | 0 (по умолчанию) | **Политика SR-B17:** утверждённое питание/термо-гидрозащита или безопасный выбег; KC868 relay не safety path; Nano duplication не default. |
| `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_1/B` | 0 | **RS485 slave**, S-49. |
| `ACT_*_VALVE_SAFETY` | 0 | **Только Nano.** |

**Вывод:** 6 onboard реле KC868 недостаточно/непригодны как safety path для полного набора актуаторов и не fail-safe. Основная силовая коммутация — **внешняя DIN/щитовая** (реле/контакторы/SSR) с Nano cutoffs. Нагрузка на свободные GPIO KC868 определяется pinmap (`SEN_FLAME` direct GPIO, safety UART, RS485/DE где нужно, SSR control, 1-Wire, диагностика).

#### Входы и интерфейсы Main Unit
| Элемент | Интерфейс | Ресурс (оценка) | Примечание |
|---|---|---:|---|
| `SEN_FLAME` | Фоторезистор -> аналоговый вход KC868 (PROPOSED, BLK-005) | **1 ADC** (кандидат) | **BLK-005 PROPOSED/NOT VALIDATED.** Экранирование + PASS/FAIL ЭМС-тесты при розжиге; fallback opto/изолированный формирователь (digital/Schmitt) на direct ESP32-S3 GPIO. Отклонение от SR-B13/S-47; fail-safe пороги/диагностика/обрыв=«нет пламени» обязательны; `safety-reviewer`+`schematic-engineer` APPROVE. |
| Детект пропадания 220В | Цифровой/аналоговый через развязку (по схеме) | 1 GPIO/ADC | Main Unit; антидребезг/гистерезис; Nano не зависит для базовой отсечки. |
| DS18B20 технологические | 1-Wire | 1–2 шины (KC868 имеет 2) | До 12 датчиков сейчас/перспектива; разводка/подтяжка/экран — pinmap/schematic. |
| `SEN_THERMOCOUPLE_K` + MAX6675 | SPI | 3 (если на Main Unit) | **BLK-003 ACCEPTED RISK (ACK req.):** технологическая обратная связь горения, **не зона контроля Nano Safety**. Nano независимого триггера перегрева выхлопа не имеет. Диагностика/fault handling на Main Unit по FSM/схеме. |
| Safety UART к Nano | Dedicated UART | 2 GPIO + уровни/изоляция по §7.18 | **Не RS485.** Кандидаты GPIO — §7.16; native USB — debug. |
| RS485 доп.устройства | RS485 | встроенный порт/назначение | Master, non-safety, S-49. |

### 1.2. Safety Unit — Arduino Nano (ATmega328P)

#### Входы (инварианты)
| Элемент | Интерфейс | GPIO (оценка) | Примечание |
|---|---|---:|---|
| `SEN_AIR_PRESSURE` | Пороговый цифровой | 1 | Критичный, Nano. |
| `SEN_FLOAT_MIN` | Геркон | 1 | Прямо к Nano. |
| `SEN_FLOAT_MAX` | Геркон | 1 | Прямо к Nano. |
| `SEN_OIL_TEMP_SAFETY` | DS18B20, 1-Wire | 1 | Прямо к Nano. |
| `SEN_THERMOCOUPLE_K` + MAX6675 | SPI | 3 (если на Nano) | По BLK-003 не зона контроля Nano Safety; размещение на Nano не требуется для safety-path. Если когда-либо рассматривается — отдельное решение/бюджет. |

#### Выходы / независимые контуры
| Элемент | Назначение | GPIO (оценка) | Примечание |
|---|---|---:|---|
| `ACT_OIL_VALVE_SAFETY` | Safety-gate, NC fail-safe | 1 | Только Nano. |
| `ACT_AIR_VALVE_SAFETY` | Safety-gate, NC fail-safe | 1 | Только Nano. |
| **Общий Nano-контактор группы** | Питание `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT` | 1 (+ драйвер/feedback по схеме) | **Решение пользователя.** Независимый силовой контактор Nano, fail-safe размыкание. Покрывает SR-B4 (ТЭН, силовая цепь последовательно с ТЭН при подтверждённой схеме) и SR-B18 (подкачка, hardware block/stop fail-safe OFF). **`ACT_FAN_BLOWER` в обход** (для продувки). Не заменяет safety-gates. Номиналы/AC-категория/inrush/ЭМС/220В — `schematic-engineer`. |
| Supervised power-cycle 12В KC868 | Ключ питания KC868 (SR-B6) | 1 (+ feedback 1) | Current-limited/soft-start, 3-state feedback, no fast cycling, latch. Nano не отсекает своё питание. |
| Аппаратный RESET Main Unit | Опция | 0–1 | Изолированный ключ; не заменяет fail-safe/power-cycle логику. |
| `ACT_PUMP_BOILER` duplication | **Не default (SR-B17)** | 0 | Только по утверждённой схеме. |

#### Связь
| Элемент | Интерфейс | GPIO |
|---|---|---|
| Safety UART с Main Unit | `Serial` (D0/D1) | 2 | Конфликт с USB-прошивкой/отладкой — процедура/переключатель/альтернатива обязательны. |

**Оценка Nano:** бюджет выводов напряжённый; решения по MAX6675, power-cycle feedback, дополнительным блокировкам влияют критично. SR-B4/SR-B18/safety-gates имеют приоритет над опциями.

## 2. Оценка достаточности платформ

### 2.1. KC868-A6v3 как Main Unit — **условно достаточно / условно рекомендуется**
- **Плюсы:** DIN/12В, ESP32-S3 N16R8, native USB, встроенные RS485/RS232/1-Wire/ADC/RTC/OLED, компактность.
- **Критичные ограничения/условия:** реле/опто-входы через PCF8574/I2C **не независимый/fail-safe safety path**. Onboard реле допустимы **только для прикладного управления** при Nano hardware safety-gate/cutoff/block-stop (BLK-004). Входы KC868 — технологические/не safety-критичные. `SEN_FLAME` — **BLK-005 PROPOSED/NOT VALIDATED** (отклонение от SR-B13/S-47). `SEN_THERMOCOUPLE_K` — BLK-003 (не зона Nano). Nano safety layer обязателен; SR-B6/S-48/§7.18/§11 открыты.
- **Вердикт:** принимать как Main Unit **только при выполнении §7/§14, тестах §10, подтверждениях §11 и ревью `schematic-engineer`+`safety-reviewer`**. Как безусловная замена — **нет**. Nano и внешние силовые/защитные цепи обязательны.

### 2.2. Arduino Nano (Safety Unit) — **условно достаточно, бюджет I/O на грани**
- Подходит для минимального safety набора при аккуратном pinmap и приоритетах SR-B4/SR-B18/safety-gates/UART/sensors.
- Конфликт D0/D1 с USB; дефицит при MAX6675/power-cycle feedback/опциях. Нужен явный бюджет в `docs/hardware/pinmap.md`.

## 3. Критичные вопросы для шага 3 (pinmap/schematic)

- [ ] Закрыть KC868 §7.16: точные свободные GPIO/strapping/взаимоисключения; назначить ресурс `SEN_FLAME` (BLK-005, предложенный ADC + fallback direct GPIO), safety UART, SSR control/диагностику.
- [ ] Grounding/isolation (§7.18): выбрать A/B до схемы, не смешивать барьеры; уровни UART Nano↔KC868; обратная связь `KC868_12V`.
- [ ] Схема safety-gate: только Nano; запрет обхода Main Unit; NC fail-safe; подтверждение контактов/драйверов (S-48).
- [ ] `ACT_HEATER_OIL`/`ACT_PUMP_OIL_TRANSFER`/`ACT_IGNITION_UNIT`: **общий Nano-контактор** (BLK-002 design gate: SR-B4/SR-B18, feedback fail-safe полярности, weld/non-trip, SR-B14/B15/SR-B12).
- [ ] `ACT_FAN_BLOWER`/board-off: Nano обесточивает **всю плату KC868** (BLK-001/SR-B6) — режимный анализ продувка/авария, side effects.
- [ ] `ACT_PUMP_BOILER` (BLK-007): KC868 relay + опц. дублированное включение Nano, автомат; оставание ON в аварии допустимо по решению пользователя; схема pending.
- [ ] `SEN_FLAME` (BLK-005): схема+алгоритм fail-safe, PASS/FAIL ЭМС-тесты при розжиге, fallback opto/изолированный формирователь; `safety-reviewer`+`schematic-engineer` APPROVE.
- [ ] `SEN_THERMOCOUPLE_K`+MAX6675 (BLK-003): технологическая обратная связь, не зона Nano; `safety-reviewer` ACK границы ответственности.
- [ ] Nano UART D0/D1: процедура прошивки/переключение/альтернатива.
- [ ] 1-Wire: разводка 12 датчиков, подтяжка, экранирование, помехи от розжига.
- [ ] RS485: сегменты DMX/Modbus (S-49), `DE/RE` runtime, терминирование, изоляция, slave watchdog/fail-safe defaults (`docs/protocol/rs485.md`).
- [ ] Supervised power-cycle KC868 (SR-B6): ключ 12В, soft-start/NTC, ток, feedback 3-state, forced OFF relays, min off-time, latch, Nano отдельное питание.
- [ ] Nano fail-safe outputs (SR-B14/SR-B15): уровни при reset/brownout/bootloader, WDT, отпускание катушек/общий внешний watchdog — схемное условие.
- [ ] 220В монтаж: DIN вне отсека котла, IP/clearance/creepage, УЗО type A 30mA, предохранители, транзиенты, NO-контакты/силовые цепи (S-48, §5.8 KC868 analysis).
- [ ] Протокол: миграция `docs/protocol/uart.md` + `include/common_protocol.h` (SR-B12/B14/B16, `fsmState/stateAge/cycles/i2cStatus/safetyPermit/gateState/alarm/latch/sensorValid/seq/protoVer/CRC`) — только с `safety-reviewer`.

## 4. Опциональный вариант: FPC (влияние на I/O при внедрении)

Статус: **не принят**. При внедрении (только гибрид): Nano сохраняет direct `SEN_OIL_TEMP_SAFETY`/`SEN_FLOAT_MIN/MAX`, **series cutoff ТЭН (SR-B4)**, **hardware block/stop подкачки (SR-B18)**. Пересчёт I/O/power/safety-gate линий — отдельное решение `schematic-engineer` + `safety-reviewer`.

## 5. Вывод

- **KC868-A6v3:** ресурсы для координации достаточны, но onboard safety/fail-safe **нет**; как Main Unit допустима **только условно** (§7/§14/§10/§11 + ревью). Основные актуаторы — внешние + Nano.
- **Nano:** функционально жизнеспособен как Safety Unit, но I/O/схема **на грани**; SR-B4/B18/safety-gates/UART/датчики приоритетны.
- **Safety link:** dedicated point-to-point UART. RS485 — non-safety периферия, S-49.
- **Следующий шаг:** закрыть pinmap/schematic условия, синхронизировать протокол, провести `safety-reviewer` (обязательно) и `schematic-engineer`.

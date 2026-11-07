# Пин-мап

Статус: **НЕ ЗАФИКСИРОВАН (шаг 3)** — заполнение только после схемы, подтверждения GPIO/strapping/grounding и ревью `schematic-engineer` + `safety-reviewer`.  
Main Unit (архитектурное решение, safety-gated): **KC868-A6v3, ESP32-S3-WROOM-1 N16R8** — см. `docs/hardware/kc868-a6v3_analysis.md` §7/§14. Safety Unit: **Arduino Nano (ATmega328P)** — независимый, не заменяется.

**Правило:** Назначение пинов **только** после обсуждения и фиксации здесь. Перед назначением — проверить достаточность выводов, коллизии, strapping/boot pins, уровни, развязку, fail-safe уровни при reset/brownout/Hi-Z.

## Источники
- `rnd/peripherals.md` — перечень периферии (источник правды)
- `docs/architecture/modules.md` — роли/инварианты
- `docs/hardware/periph_distribution.md` — распределение
- `docs/hardware/io_analysis.md` — бюджет I/O
- `docs/hardware/kc868-a6v3_analysis.md` — условия KC868 (§7/§11/§14)
- `docs/protocol/uart.md`, `docs/protocol/rs485.md`
- Даташиты/схемы подтверждённых модулей (A6v3 схема — открытый вопрос §11)

## Обязательные условия до фиксации

- [ ] **KC868 §7.16:** подтвердить точные свободные GPIO, strapping/boot pins, взаимоисключения (RS485/RS232/UART/1-Wire/ADC/JTAG/USB/native).
- [ ] **Grounding/isolation §7.18:** выбрать архитектуру A/B до схемы; не смешивать барьеры. Определить уровни/изоляцию UART Nano↔KC868, обратную связь `KC868_12V present/absent/fault`.
- [ ] **`SEN_FLAME` (BLK-005, PROPOSED/NOT VALIDATED):** пользовательское решение — фоторезистор на аналоговый вход KC868 + экранирование + PASS/FAIL ЭМС-тесты при розжиге; fallback — опторазвязка/изолированный формирователь (digital/Schmitt) на **direct ESP32-S3 GPIO**. **Отклонение от SR-B13/S-47** (ADC-raw). До `safety-reviewer`+`schematic-engineer` APPROVE не реализовывать как safety-утверждённое; fail-safe пороги/диагностика/обрыв=«нет пламени» обязательны.
- [ ] **Safety UART:** dedicated point-to-point KC868↔Nano, не RS485. Native USB — debug. Nano D0/D1 — процедура прошивки/переключение/альтернатива.
- [ ] **Nano hardware paths:** safety-gates (только Nano), общий Nano-контактор `ACT_HEATER_OIL`/`ACT_PUMP_OIL_TRANSFER`/`ACT_IGNITION_UNIT` (BLK-002 gate), SR-B6 whole-board power-off KC868 (coarse mitigation для BLK-001/004). KC868 PCF8574 relay не safety path.
- [ ] **Supervised power-cycle KC868 (SR-B6, BLK-001/004):** ключ 12В, soft-start/current limit, 3-state feedback, forced OFF relays где возможно, min off-time, latch; Nano отдельное некоммутируемое питание. Влияет на продувку/логику при board-off.
- [ ] **Nano fail-safe outputs (SR-B14/SR-B15):** уровни при reset/brownout/bootloader, Hi-Z=de-energized, WDT, отпускание катушек/общий внешний watchdog (§5.7/§7.11 analysis) — схемное условие.
- [ ] **220В/монтаж (S-48):** DIN вне отсека котла, IP/clearance/creepage, УЗО type A 30mA, предохранители, transient suppression, NO-контакты/силовые цепи по схеме.
- [ ] **RS485 (S-49):** DMX512/Modbus — раздельные сегменты; runtime `DE/RE`/timeouts/abort; slave watchdog/fail-safe defaults.
- [ ] **`SEN_THERMOCOUPLE_K`+MAX6675 (BLK-003):** технологическая обратная связь горения, **не зона контроля Nano Safety** (ACCEPTED RISK, `safety-reviewer` ACK req.).
- [ ] **`ACT_PUMP_BOILER` (BLK-007):** KC868 relay + возможно дублированное включение Nano; оставание ON в аварии не проблема; схема/защита автоматом pending.
- [ ] **`include/common_protocol.h` / `docs/protocol/uart.md`:** BLK-006 safety draft создан; **требуется `safety-reviewer` APPROVE до реализации в прошивке**.

## Safety Unit — Arduino Nano (`env:arduino_safety`)

Назначение фиксируется здесь после подтверждения бюджета. Приоритет: прямые safety sensors, safety-gates, heater cutoff (SR-B4), pump block/stop (SR-B18), safety UART, power-cycle control/feedback, WDT/fail-safe outputs.

| Пин | Функция | Назначение | Тип/уровень/развязка | Fail-safe | Примечание |
|---|---|---|---|---|---|
| D0 (RX) | UART RX / USB conflict | Safety UART (кандидат) | TBD | TBD | Конфликт с программированием — процедура обязательна |
| D1 (TX) | UART TX / USB conflict | Safety UART (кандидат) | TBD | TBD | — |
| ... | ... | ... | ... | ... | Заполнять после схемы |

## Main Unit — KC868-A6v3 (`env:esp32s3_core`, планируется; legacy `env:esp32_core` до миграции)

KC868 onboard реле/опто-входы (PCF8574/I2C) — **не независимый/fail-safe safety path**. Реле допустимы **только для прикладного управления** при Nano hardware safety-gate/cutoff/block-stop в силовых цепях (аппаратный backup, Nano приоритет). Опто/ADC входы — **только технологические/не safety-критичные**, кроме `SEN_FLAME` (BLK-005 PROPOSED/NOT VALIDATED: фоторезистор->аналоговый вход KC868, отклонение от SR-B13/S-47, требует APPROVE). Это **не заменяет** Nano safety outputs/прямые safety sensors. Конкретные GPIO — после §7.16/схемы.

| Ресурс/кандидат | Функция | Назначение | Интерфейс/модуль | Fail-safe | Примечание |
|---|---|---|---|---|---|
| Native USB-C | Debug/прошивка | Отладка | USB | N/A | Не safety link |
| ADC/GPIO (TBD §7.16, BLK-005) | `SEN_FLAME` input (PROPOSED) | Фоторезистор -> аналоговый вход KC868 (экранирование, ЭМС-тесты); fallback: изолированный формирователь -> direct GPIO | Analog+filter/diag или digital isolated forming | Loss/invalid/saturated/stuck = no flame | **Отклонение от SR-B13/S-47, NOT VALIDATED**; `safety-reviewer`+`schematic-engineer` APPROVE |
| Direct GPIO UART (TBD) | Safety UART TX/RX | KC868↔Nano | UART, уровни/изоляция по §7.18 | Link loss => safe state | Не RS485 |
| Onboard RS485 | Non-safety peripherals | `ACT_DAMPER_ACTUATOR`, `ACT_PUMP_FLOOR_*` (S-49) | RS485 master | Slave safe defaults | Отдельные сегменты DMX/Modbus |
| 1-Wire 1/2 (TBD) | DS18B20 | Технологические датчики | 1-Wire | Non-safety | S-49 runtime limits |
| PCF8574 relay 1–6 | Прикладное управление (условно безопасно) | Рабочие актуаторы **только с Nano cutoffs/gates/block-stop** | I2C `0x24` | Не fail-safe само по себе; Nano backup обязателен | Решение пользователя; не safety path |
| ... | ... | ... | ... | ... | Заполнять после схемы |

## Чек-лист при заполнении

- [ ] Все назначенные пины/ресурсы не конфликтуют (strapping, boot, USB, RS485/RS232, JTAG, I2C).
- [ ] Safety UART выделен отдельно; уровни/изоляция соответствуют §7.18.
- [ ] `SEN_FLAME` (BLK-005) — **PROPOSED/NOT VALIDATED**: фоторезистор на аналоговый вход KC868 + экранирование + PASS/FAIL ЭМС-тесты при розжиге; fallback opto/изолированный формирователь на direct ESP32-S3 GPIO. До APPROVE не safety-утверждено; fail-safe диагностика обязательна.
- [ ] `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_MIN/MAX` — прямые входы Nano.
- [ ] Safety-gates — только Nano; Main Unit не имеет обхода.
- [ ] **Общий Nano-контактор группы** (BLK-002): питание `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT` — независимый, Nano-only, fail-safe размыкание. Подтвердить SR-B4/SR-B18, номиналы/AC-категорию/inrush/ЭМС, отсутствие шунтирования, reset/brownout/Hi-Z/WDT, **feedback fail-safe полярности**, weld/non-trip анализ, latch. `ACT_FAN_BLOWER` — KC868 relay + Nano может обесточить всю плату KC868 (BLK-001/SR-B6).
- [ ] `ACT_HEATER_OIL` — внешний SSR/драйвер + термостат/термопредохранитель/предохранители; SSR не средство аварийного отключения.
- [ ] `ACT_PUMP_BOILER` (BLK-007) — KC868 relay + опц. дублированное включение Nano; оставание ON в аварии допустимо по решению пользователя; схема/приоритеты/автомат/изоляция зафиксированы.
- [ ] 220В коммутация — внешние модули/развязка/предохранители/УЗО/транзиенты (S-48). KC868 onboard relays допустимы для прикладного управления **только при Nano hardware safety-gate/cutoff/block-stop**; не safety path.
- [ ] RS485 slave устройства имеют watchdog/fail-safe defaults; сегменты S-49 соблюдены.
- [ ] Nano fail-safe output levels при reset/brownout/bootloader подтверждены (SR-B15).
- [ ] Supervised power-cycle KC868 (SR-B6) и feedback зафиксированы.
- [ ] Протокол/`common_protocol.h` миграция согласована с `safety-reviewer`.
- [ ] `schematic-engineer` + `safety-reviewer` ревью выполнен.

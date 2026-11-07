# Протокол обмена Main Unit (KC868-A6v3) <-> Safety Unit (Nano)

Статус: **черновик (требует миграции/согласования)**. Базовый вариант — **dedicated point-to-point UART** (safety-критичный канал). Общая шина RS485 для safety-связи **не рекомендуется** без отдельного обоснования, доказательства fail-safe при всех отказах шины/устройств и `APPROVE` `safety-reviewer`. KC868/архитектурные условия — `docs/hardware/kc868-a6v3_analysis.md` §7.8/§7.15/§14. **Любое изменение транспорта/формата/семантики — fail-safe анализ + обязательный `safety-reviewer`.**

## Параметры связи (планируемые, фиксировать в pinmap/прошивке)

- **Интерфейс:** выделенный UART точка-точка.
- **Main Unit (KC868, ESP32-S3):** UART на свободных GPIO-кандидатах (TBD, шаг 3, §7.16). **Native USB-C** — debug/прошивка, не safety link.
- **Safety Unit (Nano):** `Serial`/аппаратный UART (D0/D1 конфликтуют с USB-программированием/отладкой — обязательна процедура/переключение/альтернатива в pinmap/schematic). SoftwareSerial для safety-канала нежелателен.
- **Скорость:** **единая, ≤19200 бод** (норматив для надёжности/детерминизма на данном этапе; текущие 115200/9600 в заготовках — **недопустимое расхождение**, требует миграции). Финальное значение — после тестов/схемы.
- **Гальваноразвязка/уровни/grounding:** определяется архитектурой §7.18 (вариант A/B, не смешивать барьеры). 74LVC1T45/2T45 — **транслятор уровней при общей земле, не изолятор**; развязка — ISO7421/Si862x/ADuM1201/оптопары по схеме.
- **RS485:** для safety-канала не использовать. RS485 — только non-safety доп.устройства (S-49).

## Принципы

- **Простота и предсказуемость** — минимальный, валидируемый протокол.
- **Heartbeat** — обязательный, период **100–200 мс**, таймаут потери связи **≤3 периодов heartbeat (≤600 мс)**. Heartbeat формируется в цикле FSM Main Unit (не отдельным «живым» таймером), период шага FSM ≤ периода heartbeat.
- **Fail-safe (обе стороны):** таймаут/ошибка CRC/`seq`/`protoVer`/битый пакет/`I2C_FAULT`/семантическое застревание/невалидное разрешение/неопределённость => safe state. Nano закрывает safety-gate и переводит независимые контуры в fail-safe; Main Unit снимает опасные команды/запрещает пуск. Потеря связи **латчится**; восстановление связи **не возобновляет** опасные режимы без продувки/проверок и ручного подтверждения по политике.
- **Односторонний приоритет защиты:** Nano может аварийно заблокировать/перевести в safe state независимо от Main Unit. Main Unit не может обойти Nano.
- **Явное разрешение:** Nano передаёт `safetyPermit`/gate/latch/alarm/sensorValid. Отсутствие/невалидность разрешения = safe state Main Unit (нельзя отличать «жив, но запрещено» от «разрешено» без явного поля).
- **Изоляция от RS485 доп.устройств:** safety-канал не зависит от трафика/состояния RS485 (DMX/Modbus/slave).
- **Версионирование:** `protoVer` обязателен; несовместимость => safe state/запрет пуска.

## Формат пакетов (BLK-006, safety draft)

`include/common_protocol.h` содержит **safety-oriented draft** (`SafetyProtocol::HeartbeatMainToSafety` / `HeartbeatSafetyToMain`, `protoVer/seq/CRC` placeholder). Статус: **SAFETY DRAFT — не использовать в прошивке до `safety-reviewer` APPROVE**. При реализации зафиксировать: алгоритм CRC (и seed/polynomial), endian, capped ranges, точные enum/bits, обработку unknown/reserved, версионирование, тест-векторы.

### Main Unit -> Nano (минимум)
- `protoVer`, `seq` (монотонный, фиксированной ширины, переполнение по модулю)
- `fsmState`, `stateAge` (saturated/capped)
- `fsmCycle`, `i2cPollCycle` (инкремент по правилам §7.8: `fsmCycle` в конце завершённого шага FSM; `i2cPollCycle` только при успешной транзакции+read-back)
- `i2cStatus` / health PCF8574 (OK/fault/recovered)
- `actBits` — запрошенные состояния критичных актуаторов (не заменяет Nano authority)
- `flameStatus` — статус пламени/достоверности для Nano супервизии (BLK-005: реализация датчика пламени не утверждена; `FLAME_DEGRADED/INVALID` трактовать консервативно)
- `errSeverity` / диагностика
- CRC

### Nano -> Main Unit (минимум)
- `protoVer`, `seq`
- `safetyPermit` / `gateState` (явное разрешение/состояние safety-gate)
- `alarmBits`, `latch`, `errSeverity`
- `sensorValid` / статусы критичных датчиков Nano (давление, уровни, температура safety)
- `contactorFb` — feedback общего Nano-контактора (fail-safe полярность, BLK-002)
- `kc868PowerFb` — feedback питания KC868 `present/absent/fault-unknown` (SR-B6 / BLK-001/004 coarse mitigation)
- CRC

## Семантика отказов (SR-B8/B11/B16)

- **Два класса:** (1) зависание/brownout/reset — покрывается WDT/heartbeat/счётчиками/уровнями fail-safe; (2) залипание/сварка контакта/несостоявшаяся отсечка — **не** покрывается WDT/heartbeat, требует второго контакта/feedback/диагностики и latch. Меры дополняют друг друга.
- **Семантическое застревание:** живой heartbeat/растущие счётчики недостаточны; Nano проверяет допустимость `fsmState`/`stateAge` и max-таймауты состояний.
- **`I2C_FAULT` при живом heartbeat:** Nano считает Main Unit потерявшим надёжное управление KC868 onboard path и применяет fail-safe политику (safety-gate закрыт; power-cycle KC868 — только по SR-B6 правилам).
- **Потеря `safetyPermit` при живом heartbeat:** Main Unit немедленно в safe state, latch.

## KC868-специфичные примечания

- Onboard relays/inputs KC868 — PCF8574/I2C, **не safety path**. I2C health должен отражаться в протоколе; I2C recovery best-effort не заменяет Nano hardware cutoffs.
- Supervised power-cycle 12В KC868 — действие Nano (SR-B6), не команда Main Unit. Main Unit должен корректно переживать обесточивание/возврат (safe boot, no auto-restart dangerous state).
- `SEN_FLAME` (BLK-005, **PROPOSED/NOT VALIDATED**): **baseline SR-B13/S-47** — direct ESP32-S3 GPIO + внешний гальванически изолированный формирователь digital/Schmitt, fail-safe полярность/диагностика; запрещено KC868 opto/PCF8574/I2C/ADC-raw. Пользовательское отклонение (фоторезистор -> аналоговый вход KC868 + экранирование + ЭМС-тесты; fallback opto/изолированный формирователь) **НЕ утверждено и НЕ разрешено к реализации** до PASS валидации и `safety-reviewer`+`schematic-engineer` APPROVE. До APPROVE действуют требования SR-B13/S-47. `flameStatus` в heartbeat используется Nano для супервизии (`INVALID/DEGRADED` = консервативно/не пламя для опасных действий); Nano не имеет прямого входа пламени.

## Тесты/проверки (минимум, до приёмки протокола)

- Потеря UART/обрыв/short/шум => обе стороны safe state, Nano latch.
- CRC/seq/protoVer fault => safe state.
- Зависание FSM при живом heartbeat (счётчики/`stateAge`) => Nano gate closed; TWDT Main Unit.
- `I2C_FAULT` + живой heartbeat => Nano fail-safe политика.
- Потеря `safetyPermit` => Main Unit safe state/latch.
- Восстановление связи => **нет** авто-возобновления опасных режимов.
- Совместимость с Nano D0/D1 процедурой прошивки.

## Ссылки
- `docs/hardware/kc868-a6v3_analysis.md` §7.8/§7.15/§14
- `docs/architecture/modules.md`, `docs/hardware/periph_distribution.md`, `docs/hardware/pinmap.md`
- `include/common_protocol.h` — миграция обязательна (`safety-reviewer`)
- `docs/protocol/rs485.md`, `docs/protocol/errors.md` (план)
- `docs/safety/checklist.md`

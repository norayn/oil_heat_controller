# BLK-002 — brief на ревью (schematic-engineer + safety-reviewer)

Статус: **REQUEST FOR REVIEW — НЕ ЗАКРЫВАЕТ BLK-002**.  
Дата: 2026-11-08.  
Связь: `docs/architecture/blockers.md` (BLK-002, **CRITICAL DESIGN GATE**), `docs/safety/blk002_closure_package.md` (чек-лист/FMEA/evidence), `docs/safety/checklist.md`, `docs/architecture/modules.md`, `docs/hardware/{pinmap,periph_distribution,io_analysis}.md`, `docs/protocol/uart.md`, `include/common_protocol.h` (BLK-006 draft).

## Цель
Подготовить и согласовать пакет артефактов, **необходимый для возможного будущего закрытия BLK-002**. Этот документ **не является** evidence, **не является** APPROVE и **не переводит** BLK-002 в CLOSED/APPROVED.

## Текущий статус BLK-002 (факт, 2026-11-08)
- BLK-002 = **DESIGN GATE (WELD RESIDUAL RISK DRAFT, ACK REQUIRED)**.
- Область: общий Nano-управляемый силовой cutoff/block-stop (контактор/силовое реле, **не самозамыкающийся, без самоподхвата**, управление **только Nano**, **без шунтирования/дублирующих путей**) для `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT`. `ACT_FAN_BLOWER` — в обход (BLK-001 ACCEPTED DESIGN).
- Пользовательские решения зафиксированы как draft/intended: не самозамыкающийся, только Nano, без bypass/duplicate feed paths; остаточный риск залипания/сварки/несостоявшейся отсечки принят как **draft** (без отдельной weld-mitigation в текущем объёме). **Это НЕ устраняет физический риск и НЕ является `safety-reviewer` ACK.**
- Закрытие возможно **только** при выполнении всех условий из `blk002_closure_package.md` §«Критерий закрытия»: (1) утверждённая схема, (2) FMEA/residual risk с явным `safety-reviewer` ACK draft риска залипания, (3) подтверждённые компенсирующие меры, (4) evidence/тесты до опытной эксплуатации. Требуются `schematic-engineer` + `safety-reviewer` APPROVE.

## Nano-инварианты (нельзя ослаблять)
- Safety-gates NC `ACT_OIL/AIR_VALVE_SAFETY` — **только Nano**, независимые контуры, fail-safe закрыты.
- `SEN_AIR_PRESSURE`, `SEN_OIL_TEMP_SAFETY`, `SEN_FLOAT_MIN/MAX` — **прямые на Nano**.
- SR-B4/SR-B18 — **только подтверждённой схемой** (SSR не аварийное отключение; Nano cutoff последовательно с ТЭН; hardware block/stop подкачки fail-safe OFF).
- SR-B14/B15/SR-B12/S-42: fail-safe outputs Nano, определённые уровни при reset/brownout/Hi-Z/WDT/bootloader; de-energize-to-trip где применимо.
- Fail-safe, latch, **no auto-resume/restart** опасных режимов без безопасных условий/проверок/политики подтверждения.

## Что требуется от schematic-engineer (схемный пакет)
Подготовить схемы/спецификации (без финализации pinmap/schematic как «утверждено» до approvals). Минимум:

1. **Топология общей группы (BLK-002/SR-B4/SR-B18):** источник питания, ветви/защита (номиналы — отдельный шаг, но характеристики/категории указать), Nano cutoff контакт(ы), разводка на `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT`. **Нет самоподхвата** (без удерживающих контактов/self-hold). **Нет шунтирования** (Main Unit/KC868/SSR/альтернативная проводка не подают питание в обход). `ACT_FAN_BLOWER` подтверждённо в обход (BLK-001).
2. **SR-B4 (ТЭН):** внешний SSR/драйвер (прикладное, **не** аварийное отключение) + Nano cutoff **последовательно с ТЭН**; фазировка/полюса/защитные элементы; **термовыключатель/«теплушка» на корпусе нагреваемого бака** + термопредохранитель/предохранители. Зафиксировать: место установки, тепловую связь, уставку/гистерезис, среду, номиналы по току/напряжению, fail-safe направление (размыкание при перегреве). **Тип сброса теплушки (ручной/самовозврат) — явная фиксация для safety-review ACK** и совместимость с latch/no auto-resume.
3. **SR-B18 (подкачка):** hardware block/stop fail-safe OFF; нет обхода KC868 relay/FPC/проводкой.
4. **Ignition unit:** обесточивание в составе группы; учесть ЭМС/наводки ВВ розжига и подавление транзиентов.
5. **Управление/драйвер Nano (SR-B14/B15/SR-B12/S-42):** таблица состояний (normal ON/OFF, Nano fault, Nano power loss, driver fault, feedback fault); reset/brownout/Hi-Z/WDT/bootloader => de-energize; pull-up/pull-down, active polarity, защита от ложного включения. Nano **не** режет собственное питание.
6. **Feedback (fail-safe полярность):** физическая реализация (aux/mirror/voltage sense) без шунтирования силовой цепи; состояния `OFF_CONFIRMED / ON_CONFIRMED / UNKNOWN / FAULT` (или эквивалент с обоснованием); **обрыв/КЗ/потеря питания/недостоверность = UNKNOWN/FAULT, не OFF_CONFIRMED**.
7. **220В/монтаж (S-48/BLK-009):** DIN вне отсека котла, IP/clearance/creepage, защита ветвей, транзиенты, селективность/автоматика по схеме. Номиналы — отдельный шаг; выбор не снимает gates.
8. **Характеристики нагрузки:** AC-категория/тип контакта для резистивной (ТЭН), двигательной (подкачка) и специфической (розжиг) нагрузки; inrush/пусковые токи; ЭМС.

## Что требуется от safety-reviewer (review/ACK пакет)
1. **Topology review:** подтверждение отсутствия self-hold/bypass/shunt; fail-safe размыкание при отказах Nano/драйвера; соответствие Nano-инвариантам.
2. **SR-B4/SR-B18 review:** SSR не аварийное отключение; cutoff последовательно с ТЭН; теплушка/термопредохранители действительно ограничивают нагрев при отказах (в т.ч. в weld residual risk сценарии) только в пределах подтверждённой схемы; **тип сброса теплушки — явная фиксация + ACK**, совместимость с latch/no auto-resume.
3. **FMEA/residual risk:** режимы залипания/сварки контакта, неполного размыкания, залипания драйвера, ложного/потерянного feedback, self-hold (исключён схемой), шунтирования (исключён); эффект/обнаружение/компенсация/остаточный риск. Явная запись: **residual weld/non-opening risk accepted as draft — requires `safety-reviewer` ACK**. Риск **не устранён физически**. Последствия «нагрузка запитана при закрытых safety-gates» для ТЭН/подкачки/розжига: ТЭН ограничен теплушкой/термопредохранителями; подкачка/розжиг не дают опасную подачу топлива/воздуха благодаря Nano safety-gates + `SEN_AIR_PRESSURE` (подтвердить схемой/FMEA).
4. **Feedback semantics:** `UNKNOWN/FAULT` != `OFF_CONFIRMED`; поведение Nano = latch, запрет повторной подачи/опасного рестарта.
5. **Protocol alignment (BLK-006, если затрагивается):** `contactorFb`/статусы в heartbeat консервативны; любые изменения `include/common_protocol.h`/`docs/protocol/uart.md` — отдельный `safety-reviewer` APPROVE до прошивки.
6. **Явный вердикт:** APPROVE / APPROVE_WITH_CONDITIONS / REQUEST_CHANGES / BLOCK. **ACK residual weld risk** и **ACK типа сброса теплушки** — отдельные явные решения, только при наличии схемы+FMEA+компенсаций.

## Evidence / тесты (до возможного закрытия BLK-002 и до опытной эксплуатации)
1. Power-on/reset/brownout/Hi-Z/WDT/bootloader Nano/драйвера: cutoff разомкнут (стенд/осциллограф/лог).
2. Команда Nano OFF при нормальном питании: `ACT_HEATER_OIL`, `ACT_PUMP_OIL_TRANSFER`, `ACT_IGNITION_UNIT` обесточены; `ACT_FAN_BLOWER` — по BLK-001 схеме (в обход).
3. Feedback fail-safe: обрыв/неисправность/потеря питания feedback => `UNKNOWN/FAULT`, не `OFF_CONFIRMED`; Nano -> latch/no restart.
4. Weld/stuck/non-trip: практическая симуляция где безопасно применимо, иначе обоснованный анализ + подтверждение поведения (latch, запрет рестарта, safety-gates блокируют опасную подачу).
5. SR-B4: проверка цепи ТЭН, отсутствие обхода SSR/контактора/теплушки/термопредохранителя; безопасная методика проверки теплушки на корпусе бака.
6. SR-B18: подтверждение fail-safe OFF подкачки.
7. `safety-reviewer` evidence review.

## Готовый запрос: schematic-engineer
```text
Задача: подготовить схемный пакет для BLK-002 (CRITICAL DESIGN GATE, НЕ закрывать без approvals).
Область: общий Nano-управляемый cutoff/block-stop (контактор/силовое реле, НЕ самозамыкающийся, без самоподхвата, только Nano, без шунтирования) для ACT_HEATER_OIL, ACT_PUMP_OIL_TRANSFER, ACT_IGNITION_UNIT. ACT_FAN_BLOWER в обход (BLK-001).
Требования: SR-B4 (SSR не аварийное отключение + cutoff последовательно с ТЭН + термовыключатель/«теплушка» на корпусе нагреваемого бака + термопредохранитель/предохранители; тип сброса теплушки зафиксировать для safety-review ACK), SR-B18 fail-safe OFF, ignition в группе, Nano driver reset/brownout/Hi-Z/WDT/bootloader => de-energize (SR-B14/B15/SR-B12/S-42), feedback fail-safe полярности (UNKNOWN/FAULT != OFF_CONFIRMED), без шунтирования/self-hold, Nano не режет своё питание, 220В/S-48/BLK-009, AC-категория/inrush/ЭМС (номиналы — отдельный шаг).
Артефакты: силовая схема, схема управления/feedback, таблица состояний fail-safe, места установки/параметры теплушки (тип сброса!), монтажные заметки 220В, входные данные для FMEA.
Ограничение: не финализировать pinmap/schematic как «утверждено»; требуется safety-reviewer + schematic-engineer согласование.
```

## Готовый запрос: safety-reviewer
```text
Задача: safety review пакета BLK-002 (CRITICAL DESIGN GATE). НЕ утверждать закрытие BLK-002 без полной схемы, FMEA/ACK и evidence.
Проверить: Nano-инварианты сохранены; non-self-latching/no self-hold/no bypass подтверждены; SR-B4 (SSR не аварийное отключение, cutoff последовательно с ТЭН, теплушка на корпусе бака + термопредохранители; тип сброса теплушки — явная фиксация и совместимость с latch/no auto-resume); SR-B18 fail-safe OFF; Nano reset/brownout/Hi-Z/WDT/bootloader fail-safe; feedback fail-safe полярность (UNKNOWN/FAULT != OFF_CONFIRMED) => latch/no restart.
FMEA: weld/non-opening/driver/feedback/self-hold/shunt; последствия запитанной нагрузки при закрытых safety-gates. Отдельно оценить draft: «residual weld/non-opening risk accepted» — физический риск НЕ устранён; дать явный ACK/не ACK (только при компенсирующих мерах: safety-gates NC + SEN_AIR_PRESSURE, SR-B4 thermal chain, feedback/latch/no restart) и явный ACK/не ACK типа сброса теплушки.
Вердикт: APPROVE / APPROVE_WITH_CONDITIONS / REQUEST_CHANGES / BLOCK + явный статус weld-risk ACK и thermal-switch reset-type ACK.
```

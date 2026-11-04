# Агенты проекта oil_heat_controller

Роли прописаны в `.opencode/agent/`. Их можно активировать через инструмент `subagent`.

## Доступные роли
| Файл | Роль | Назначение |
|---|---|---|
| `chief-engineer.md` | `chief-engineer` | Главный инженер. Архитектура, план, координация |
| `esp32-dev.md` | `esp32-dev` | Разработчик ESP32 (src_esp32/) |
| `nano-dev.md` | `nano-dev` | Разработчик Arduino Nano (src_avr8/) - модуль безопасности |
| `schematic-engineer.md` | `schematic-engineer` | Схемотехник, пин-мап, развязка |
| `safety-reviewer.md` | `safety-reviewer` | Ревью по безопасности ВСЕХ модулей |

## Как активировать
Через `subagent` инструмент:
```json
{
  "agent": "chief-engineer",
  "description": "Шаг 1: архитектура и распределение обязанностей",
  "prompt": "Выполни шаг 1 плана из rnd/plan.txt. Опирайся на rnd/peripherals.md, AGENTS.md, docs/..."
}
```

## Старт с шага 1 от Главного Инженера
Для выполнения первого пункта плана (шаг 1) из роли Главного Инженера используй `chief-engineer` с заданием проанализировать и зафиксировать архитектуру/распределение обязанностей в `docs/architecture/modules.md`.

**Важно:** `safety-reviewer` автоматически ревьюирует все правки кода во всех модулях.

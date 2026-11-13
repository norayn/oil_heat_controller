#pragma once
// Управляемые host-часы для native-тестов. Компилируются только в native env.

#include <cstdint>

namespace platform {
namespace host {

void setNowMs(uint32_t nowMs);
void advanceMs(uint32_t deltaMs);

}  // namespace host
}  // namespace platform

#pragma once
// FIELD: manual-перехода нет. Порт существует, чтобы у Off -> Manual
// не было вызывающего; всегда возвращает false.

#include "hal/ports.h"

class NullManualRequest final : public IManualRequest {
public:
    bool requested() override { return false; }
};

#pragma once
// NVS-адаптер (INvs) поверх ESP-IDF NVS. M0 — каркас; M4 добавит версию/CRC
// и fail-closed семантику для safety-счётчиков, M6 — SettingsService.

#include <cstddef>
#include <cstdint>
#include <nvs.h>

#include "hal/ports.h"

class NvsStore final : public INvs {
public:
    ~NvsStore() override;

    bool begin(const char* nameSpace);

    bool read(const char* key, void* out, size_t cap) override;
    bool write(const char* key, const void* in, size_t len) override;

private:
    nvs_handle_t handle_{};
    bool         opened_{false};
};

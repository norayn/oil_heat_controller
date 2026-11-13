#include "platform/nvs.h"

NvsStore::~NvsStore() {
    if (opened_) {
        nvs_close(handle_);
    }
}

bool NvsStore::begin(const char* nameSpace) {
    if (nameSpace == nullptr || opened_) {
        return false;
    }
    opened_ = nvs_open(nameSpace, NVS_READWRITE, &handle_) == ESP_OK;
    return opened_;
}

bool NvsStore::read(const char* key, void* out, size_t cap) {
    if (!opened_ || key == nullptr || out == nullptr || cap == 0) {
        return false;
    }

    size_t len = cap;
    return nvs_get_blob(handle_, key, out, &len) == ESP_OK;
}

bool NvsStore::write(const char* key, const void* in, size_t len) {
    if (!opened_ || key == nullptr || in == nullptr || len == 0) {
        return false;
    }

    if (nvs_set_blob(handle_, key, in, len) != ESP_OK) {
        return false;
    }
    return nvs_commit(handle_) == ESP_OK;
}

#pragma once
// Порты HAL (§9.1). Прикладной код работает только через них.
// IClock намеренно не вводится: глобальное время — platform::nowMs().

#include <cstddef>
#include <cstdint>
#include "domain/sensor_types.h"

class IClock {
public:
    virtual ~IClock() = default;
    virtual uint32_t millis() = 0;
};

enum class Readback : uint8_t { NotAvailable = 0, Off, On, Unknown };

class IDigitalOut {
public:
    virtual ~IDigitalOut() = default;
    virtual void write(bool on) = 0;
    virtual Readback readback() const = 0;
};

class ITempBus {
public:
    virtual ~ITempBus() = default;
    virtual void requestConversion() = 0;
    virtual bool read(float& outC) = 0;
};

class IExhaustTemp {   // MAX6675 (SPI), отдельно от DS18B20-шины
public:
    virtual ~IExhaustTemp() = default;
    virtual void requestConversion() = 0;
    virtual bool read(float& outC) = 0;
};

class ILinkTransport {
public:
    virtual ~ILinkTransport() = default;
    virtual size_t read(uint8_t* buf, size_t cap) = 0;
    // Возвращает число принятых байт; частичная запись допустима и означает
    // "продолжить в следующем тике" (non-blocking drain, §6).
    virtual size_t write(const uint8_t* buf, size_t len) = 0;
};

class INvs {
public:
    virtual ~INvs() = default;
    virtual bool read(const char* key, void* out, size_t cap) = 0;
    virtual bool write(const char* key, const void* in, size_t len) = 0;
};

class IHeatDemand {
public:
    virtual ~IHeatDemand() = default;
    virtual bool active() = 0;
};

class IStopInput {
public:
    virtual ~IStopInput() = default;
    virtual bool pressed() = 0;
};

class IResetInput {
public:
    virtual ~IResetInput() = default;
    virtual bool pressed() = 0;
};

// AUTO-only: датчик пламени (BLK-005).
class IFlameInput {
public:
    virtual ~IFlameInput() = default;
    virtual FlameStatus read() = 0;
};

// Manual-переход: в FIELD — NullManualRequest (всегда false), живёт в app/.
class IManualRequest {
public:
    virtual ~IManualRequest() = default;
    virtual bool requested() = 0;
};

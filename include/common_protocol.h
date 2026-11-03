#pragma once
#include <Arduino.h>

struct __attribute__((packed)) TelemetryPacket {
    uint8_t startByte = 0xAA;
    float currentTemperature;
    uint8_t systemStatus; 
    uint8_t crc;
};

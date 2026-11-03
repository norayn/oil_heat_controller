#include <Arduino.h>
#include "common_protocol.h" // Подключаем общий заголовочный файл

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Communication Module Started");
}

void loop() {
    // Логика работы ESP32 (Wi-Fi, отправка команд)
    delay(1000);
}

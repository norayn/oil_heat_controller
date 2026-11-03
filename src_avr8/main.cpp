#include <Arduino.h>
#include "common_protocol.h" // Подключаем тот же общий файл

void setup() {
    Serial.begin(9600);
    Serial.println("Arduino Safety Module Started");
}

void loop() {
    // Логика работы Arduino
    delay(1000);
}

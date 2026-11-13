#include <Arduino.h>

// M0: каркас Safety Unit. Safety UART (include/common_protocol.h) подключается
// на этапе M3/N3, когда формат будет согласован (BLK-006). Сейчас draft не
// включаем: его static_assert'ы на AVR не проходят и к каркасу не относятся.

void setup() {
    Serial.begin(9600);
    Serial.println("Arduino Safety Module Started (M0 skeleton)");
}

void loop() {
    // Логика безопасности появится на M3/M4.
}

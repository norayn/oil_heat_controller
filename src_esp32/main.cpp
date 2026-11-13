#include "app/app.h"

// Arduino framework вызывает setup()/loop(). Вся логика — в app/.
// Serial.begin выполняет platform::logBegin() внутри app::setup(),
// чтобы earlyApplicationSafe() успел раньше Serial/I2C (§11.1).

void setup() {
    app::setup();
}

void loop() {
    app::tick();
}

#include <Arduino.h>

constexpr uint8_t PIN_LED_RED   = 4;
constexpr uint8_t PIN_LED_GREEN = 5;
constexpr uint8_t PIN_LED_BLUE  = 6;

struct Led {
  uint8_t  pin;
  uint32_t periodMs;
  uint32_t lastChange;
  bool     isOn;
};

Led leds[] = {
  { PIN_LED_RED,   200,  0, false },
  { PIN_LED_BLUE,  500,  0, false },
  { PIN_LED_GREEN, 1000, 0, false },
};


// ============================ SUPERLOOP ============================
void setup() {
  for (Led& l : leds) {
    pinMode(l.pin, OUTPUT);
  }
}

void loop() {
  const uint32_t nowMs = millis();
  for (Led& l: leds) {
    if (nowMs - l.lastChange >= l.periodMs) {
        l.lastChange = nowMs;
        l.isOn = !l.isOn;
        digitalWrite(l.pin, l.isOn);
    }
  }
}

#include <Arduino.h>

constexpr uint8_t PIN_LED_RED   = 4;
constexpr uint8_t PIN_LED_GREEN = 5;
constexpr uint8_t PIN_LED_BLUE  = 6;

struct Led {
  uint8_t  pin;
  uint32_t periodMs;
};

Led leds[] = {
  { PIN_LED_RED,   200 },
  { PIN_LED_BLUE,  500 },
  { PIN_LED_GREEN, 1000 },
};

void ledTask(void *pv) {
    Led *led = static_cast<Led *>(pv);
    pinMode(led->pin, OUTPUT);

    TickType_t last = xTaskGetTickCount();
    bool isOn = false;
    for (;;) {
        isOn = !isOn;
        digitalWrite(led->pin, isOn);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(led->periodMs));
    }
}

// ============================ SUPERLOOP ============================
void setup() {
  for (Led& l : leds) {
    xTaskCreate(ledTask, "led", 2048, const_cast<Led*>(&l), 1, nullptr);
  }
}

void loop() {

}

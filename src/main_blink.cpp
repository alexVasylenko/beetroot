#include <Arduino.h>

constexpr uint8_t LED_1    = 15;
constexpr uint8_t LED_2    = 16;
constexpr uint8_t BTN_EXT  = 21;
constexpr uint8_t BTN_BOOT = 0;

constexpr uint16_t FAST_DELAY_MS = 200;    
constexpr uint16_t SLOW_DELAY_MS = 1000;   
constexpr uint32_t DEBOUNCE_MS   = 50;     

enum Mode : uint8_t {
  MODE_SYNC      = 1,   
  MODE_ALTERNATE = 2
};

volatile Mode currentMode = MODE_SYNC;
// volatile Mode currentMode = MODE_ALTERNATE;

volatile uint32_t lastPressExt  = 0;
volatile uint32_t lastPressBoot = 0;
volatile uint32_t lastLEDChange = 0;

void onExternalButton() {
  uint32_t now = millis();
  if (now - lastPressExt < DEBOUNCE_MS) return;
  lastPressExt = now;
  currentMode = MODE_SYNC;
}

void onBootButton() {
  uint32_t now = millis();
  if (now - lastPressBoot < DEBOUNCE_MS) return;
  lastPressBoot = now;
  currentMode = MODE_ALTERNATE;
}

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  pinMode(BTN_EXT, INPUT_PULLUP);
  pinMode(BTN_BOOT, INPUT);

  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);

  attachInterrupt(BTN_EXT,  onExternalButton, FALLING);
  attachInterrupt(BTN_BOOT, onBootButton,     FALLING);
}

void loop() {
  const Mode mode = currentMode;
  uint32_t now = millis();

  if (mode == MODE_SYNC && now - lastLEDChange != FAST_DELAY_MS) return;

  if (mode ==  MODE_ALTERNATE && now - lastLEDChange != SLOW_DELAY_MS) return;

  bool isOn = digitalRead(LED_1) == 1 || digitalRead(LED_2) == 1;
  lastLEDChange = now;

  if (mode == MODE_SYNC && isOn) {
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
  }

  if (mode == MODE_SYNC && !isOn) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
  }

  bool isLED1On = digitalRead(LED_1) == 1;
  if (mode == MODE_ALTERNATE && isLED1On) {
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, HIGH);
  }

  if (mode == MODE_ALTERNATE && !isLED1On) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, LOW);
  }
}

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

//volatile Mode currentMode = MODE_ALTERNATE;

volatile uint32_t lastPressExt  = 0;
volatile uint32_t lastPressBoot = 0;
volatile uint32_t lastLEDChange = 0;

void onExternalButton() {
  uint32_t now = millis();
  if (now - lastPressExt < DEBOUNCE_MS) return;   // відсікаємо дрижання
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

  if (mode == MODE_SYNC) {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    delay(FAST_DELAY_MS);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    delay(FAST_DELAY_MS);
  } else {
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, LOW);
    delay(SLOW_DELAY_MS);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, HIGH);
    delay(SLOW_DELAY_MS);
  }


  /* uint32_t now = millis();

  static Mode reportedMode = (Mode)0;
  
  bool isModeChanged = mode != reportedMode;

  if (isModeChanged) {
    lastLEDChange = now;
  }
  if (mode == MODE_SYNC) {
    bool isOn = digitalRead(LED_1) == 1 && digitalRead(LED_2) == 1;
    if ((isOn && now - lastLEDChange == FAST_DELAY_MS) || isModeChanged) {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      lastLEDChange = now;
      return;
    }

     if ((!isOn && now - lastLEDChange == FAST_DELAY_MS) || isModeChanged) {
      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, HIGH);
      lastLEDChange = now;
      return;
    }
  }

  if (mode == MODE_ALTERNATE) {
    bool isOn = digitalRead(LED_1) == 1 && digitalRead(LED_2) == 1;
    if (isOn && now - lastLEDChange == SLOW_DELAY_MS) {
      digitalWrite(LED_1, HIGH);
      digitalWrite(LED_2, LOW);
      lastLEDChange = now;
      return;
    }

     if (!isOn && now - lastLEDChange == SLOW_DELAY_MS) {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, HIGH);
      lastLEDChange = now;
      return;
    }
  } */
}

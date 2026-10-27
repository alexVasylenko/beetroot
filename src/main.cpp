#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

constexpr uint8_t LDR_PIN   = 4;
constexpr uint8_t RELAY_PIN = 21;

#define TFT_SCK  12
#define TFT_MOSI 11
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8
#define TFT_BLK   7

constexpr uint8_t RELAY_ON  = HIGH;
constexpr uint8_t RELAY_OFF = LOW;

constexpr uint16_t THRESHOLD_DARK  = 1550;
constexpr uint16_t THRESHOLD_LIGHT = 1800;

static_assert(THRESHOLD_LIGHT > THRESHOLD_DARK,
              "THRESHOLD_LIGHT має бути більшим за THRESHOLD_DARK");

constexpr uint32_t CONFIRM_MS = 2000;
constexpr uint32_t SAMPLE_MS  = 100;
constexpr uint8_t  AVG_N      = 32;

enum AppMode : uint8_t { MODE_RUN, MODE_CALIBRATE };
AppMode appMode = MODE_RUN;

bool relayOn = false;

uint16_t seenMin = UINT16_MAX;
uint16_t seenMax = 0;

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

static uint16_t readLdr() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < AVG_N; i++) {
    sum += analogRead(LDR_PIN);
    delayMicroseconds(200);
  }
  return (uint16_t)(sum / AVG_N);
}

static void setRelay(bool on) {
  relayOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
  Serial.printf(">>> Навантаження %s\n", on ? "УВІМКНЕНО" : "вимкнено");
}

static void printHelp() {
  Serial.println();
  Serial.println("Команди: 'c' — калібрування | 'r' — робочий режим");
  Serial.printf("Пороги в прошивці: темно < %u, світло > %u (мертва зона %u відліків)\n",
                THRESHOLD_DARK, THRESHOLD_LIGHT,
                (unsigned)(THRESHOLD_LIGHT - THRESHOLD_DARK));
}

void initScreen() {
  pinMode(TFT_BLK, OUTPUT);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.init(170, 320);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.setCursor(20, 60);
}
void setup() {
  Serial.begin(115200);
  delay(300);
  initScreen();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  analogReadResolution(12);

  Serial.println("Сутінковий перемикач, ESP32-S3");
  printHelp();
}

void clearScreen(int8_t want) {
  tft.fillScreen(0);
  tft.println(want > 0 ? "Light" : "Dark");
}

void loop() {
  static uint32_t lastSample = 0;

  if (Serial.available()) {
    const char c = (char)Serial.read();
    appMode = MODE_RUN;
    Serial.println("\n=== РОБОЧИЙ РЕЖИМ ===");
  }

  if (millis() - lastSample < SAMPLE_MS) return;
  lastSample = millis();

  const uint16_t raw = readLdr();

  int8_t want = 0;
  if (!relayOn && raw < THRESHOLD_DARK)  want = +1;
  if ( relayOn && raw > THRESHOLD_LIGHT) want = -1;

  static int8_t  pending = 0;
  static uint32_t pendingSince = 0;

  if (want != pending) {
    pending = want;
    pendingSince = millis();
    if (want != 0) {
      clearScreen(want);
      Serial.printf("ADC %4u — умова %s, тримаю %lu мс...\n",
                    raw, want > 0 ? "ТЕМНО" : "СВІТЛО", (unsigned long)CONFIRM_MS);
    }
  } else if (pending != 0 && millis() - pendingSince >= CONFIRM_MS) {
    setRelay(pending > 0);
    pending = 0;
  }
}

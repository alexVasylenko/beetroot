#include <Arduino.h>

constexpr uint8_t PIN_SENSOR = 16;
constexpr uint8_t PIN_STOP   = 0;
constexpr uint8_t PIN_ALARM  = 6;
constexpr uint8_t PIN_ADC    = 4;

constexpr UBaseType_t PRIO_REPORT    = 2;
constexpr UBaseType_t PRIO_ADC       = 2;
constexpr UBaseType_t PRIO_EMERGENCY = 5;

constexpr BaseType_t CORE_0 = 0;
constexpr BaseType_t CORE_1 = 1;

constexpr uint32_t PERIOD_MS   = 1000;
constexpr uint32_t DEBOUNCE_MS = 50;
constexpr size_t   REPORT_SIZE = 192;
constexpr uint32_t STACK_BYTES = 4096;

static volatile uint32_t partsCount = 0;
static portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

static char reportBuffer[REPORT_SIZE];
static SemaphoreHandle_t reportMutex = nullptr;

static volatile bool systemHalted = false;

static TaskHandle_t hReport    = nullptr;
static TaskHandle_t hAdc       = nullptr;
static TaskHandle_t hEmergency = nullptr;

void IRAM_ATTR sensorIsr() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < DEBOUNCE_MS) return;
  lastMs = now;

  portENTER_CRITICAL_ISR(&counterMux);
  partsCount++;
  portEXIT_CRITICAL_ISR(&counterMux);
}

void IRAM_ATTR stopIsr() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < DEBOUNCE_MS) return;
  lastMs = now;

  BaseType_t higherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(hEmergency, &higherPriorityTaskWoken);

  if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
}

static void reportTask(void *pv) {
  (void)pv;
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t prevCount = 0;

  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(PERIOD_MS));
    if (systemHalted) continue;

    uint32_t count;
    portENTER_CRITICAL(&counterMux);
    count = partsCount;
    portEXIT_CRITICAL(&counterMux);

    const uint32_t perMinute = (count - prevCount) * (60000 / PERIOD_MS);
    prevCount = count;

    if (xSemaphoreTake(reportMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      snprintf(reportBuffer, REPORT_SIZE,
               "[%6lu ms] деталей: %-5lu швидкість: %-4lu дет/хв",
               (unsigned long)millis(), (unsigned long)count,
               (unsigned long)perMinute);
      xSemaphoreGive(reportMutex);
      xTaskNotifyGive(hAdc);
    } else {
      Serial.println("[Report] мʼютекс зайнятий довше 100 мс");
    }
  }
}

// --------------------- Task 2: АЦП, ядро 1 -------------------------------
static void adcTask(void *pv) {
  (void)pv;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (systemHalted) continue;

    const uint16_t mv = analogReadMilliVolts(PIN_ADC);

    if (xSemaphoreTake(reportMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      const size_t len = strlen(reportBuffer);
      snprintf(reportBuffer + len, REPORT_SIZE - len, " | АЦП: %u мВ", mv);
      Serial.println(reportBuffer);        // рядок повний — друкуємо
      xSemaphoreGive(reportMutex);
    } else {
      Serial.println("[ADC] мʼютекс зайнятий довше 100 мс");
    }
  }
}

static void emergencyTask(void *pv) {
  (void)pv;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    digitalWrite(PIN_ALARM, HIGH);
    systemHalted = true;

    detachInterrupt(digitalPinToInterrupt(PIN_SENSOR));

    if (xSemaphoreTake(reportMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      snprintf(reportBuffer, REPORT_SIZE, "*** SYSTEM HALTED! ***");
      Serial.println(reportBuffer);
      xSemaphoreGive(reportMutex);
    } else {
      Serial.println("*** SYSTEM HALTED! (буфер був зайнятий) ***");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_ALARM, OUTPUT);
  digitalWrite(PIN_ALARM, LOW);
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_STOP, INPUT);          
  analogReadResolution(12);

  Serial.println();
  Serial.println("GPIO16 — деталь | BOOT — аварійна зупинка");

  reportMutex = xSemaphoreCreateMutex();
  if (reportMutex == nullptr) {
    Serial.println("ПОМИЛКА: не вдалося створити мʼютекс");
    return;
  }

  xTaskCreatePinnedToCore(reportTask, "report", STACK_BYTES, nullptr,
                          PRIO_REPORT, &hReport, CORE_0);
  xTaskCreatePinnedToCore(adcTask, "adc", STACK_BYTES, nullptr,
                          PRIO_ADC, &hAdc, CORE_1);
  xTaskCreatePinnedToCore(emergencyTask, "emergency", STACK_BYTES, nullptr,
                          PRIO_EMERGENCY, &hEmergency, CORE_1);

  if (hReport == nullptr || hAdc == nullptr || hEmergency == nullptr) {
    Serial.println("ПОМИЛКА: не вдалося створити задачі");
    return;
  }

  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), sensorIsr, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_STOP), stopIsr, FALLING);
}

void loop() {}

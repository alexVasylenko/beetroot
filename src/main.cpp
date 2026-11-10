#include <Arduino.h>

constexpr UBaseType_t PRIO_LOW      = 1;
constexpr UBaseType_t PRIO_NORMAL   = 2;
constexpr UBaseType_t PRIO_HIGH     = 3;
constexpr UBaseType_t PRIO_REALTIME = 4;

constexpr BaseType_t APP_CORE = 1;

constexpr uint32_t SENSOR_PERIOD_MS = 500;
constexpr uint32_t EVENT_PERIOD_MS  = 3000;
constexpr uint32_t MAINT_PERIOD_MS  = 10000;
constexpr uint32_t MAINT_PAUSE_MS   = 2000;

constexpr uint32_t STACK_BYTES = 3072;

TaskHandle_t sensorHandle = nullptr;
uint32_t     workerCounter = 0;

static void logLine(const char *msg) {
  Serial.printf("[%7lu ms] %s\n", (unsigned long)millis(), msg);
}

static void sensorTask(void *pv) {
  (void)pv;

  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    logLine("[Sensor] Data read...");

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
  }
}

static void workerTask(void *pv) {
  (void)pv;

  logLine("[Worker] Processing event...");

  vTaskDelete(NULL);
}

static void eventTriggerTask(void *pv) {
  (void)pv;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(EVENT_PERIOD_MS));

    char name[configMAX_TASK_NAME_LEN];
    Serial.print(name);

    char msg[96];
    Serial.print("[Event] Створюю ");
    Serial.print(name);
    logLine(msg);

    BaseType_t ok = xTaskCreatePinnedToCore(
        workerTask, name, STACK_BYTES, nullptr, PRIO_HIGH, nullptr, APP_CORE);

    if (ok != pdPASS) {
      logLine("[Event] ПОМИЛКА: не вдалося створити задачу (мало памʼяті)");
    }
  }
}

static void maintenanceTask(void *pv) {
  (void)pv;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(MAINT_PERIOD_MS));

    vTaskSuspend(sensorHandle);
    logLine("[Maintenance] System paused for 2 seconds.");

    vTaskDelay(pdMS_TO_TICKS(MAINT_PAUSE_MS));

    vTaskResume(sensorHandle);
    logLine("[Maintenance] Sensor resumed");
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("=== Завдання 6.2: чотири задачі FreeRTOS ===");
  Serial.printf("Ядро %d, tick = %u Гц, макс. пріоритет = %d\n",
                (int)APP_CORE, (unsigned)configTICK_RATE_HZ,
                (int)configMAX_PRIORITIES - 1);
  Serial.println();

  xTaskCreatePinnedToCore(sensorTask, "sensor", STACK_BYTES, nullptr,
                          PRIO_NORMAL, &sensorHandle, APP_CORE);

  xTaskCreatePinnedToCore(eventTriggerTask, "event", STACK_BYTES, nullptr,
                          PRIO_LOW, nullptr, APP_CORE);

  xTaskCreatePinnedToCore(maintenanceTask, "maint", STACK_BYTES, nullptr,
                          PRIO_REALTIME, nullptr, APP_CORE);
}

void loop() {
}

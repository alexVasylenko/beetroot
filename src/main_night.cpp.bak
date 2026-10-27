#include <Arduino.h>

constexpr uint8_t  LDR_PIN   = 4;
constexpr uint16_t ADC_MAX   = 4095;
constexpr uint16_t U_REF_MV  = 3100;
constexpr uint32_t PERIOD_MS = 100;
constexpr uint8_t  AVG_N     = 32;

uint32_t samples      = 0;
float    errSum       = 0.0f;
float    errMax       = 0.0f;
uint16_t errMaxRaw    = 0;

constexpr uint8_t BINS = 8;
float    binErrSum[BINS] = {0};
uint32_t binCount[BINS]  = {0};

static void readAveraged(uint16_t &raw, uint16_t &mv) {
  uint32_t sumRaw = 0, sumMv = 0;
  for (uint8_t i = 0; i < AVG_N; i++) {
    sumRaw += analogRead(LDR_PIN);
    sumMv  += analogReadMilliVolts(LDR_PIN);
    delayMicroseconds(200);
  }
  raw = (uint16_t)(sumRaw / AVG_N);
  mv  = (uint16_t)(sumMv / AVG_N);
}

static void printHeader() {
  Serial.println();
  Serial.println("  №  |  RAW  | U розрах. | U виміряна |   Δ    | Похибка");
  Serial.println("     |       |    мВ     |     мВ     |   мВ   |    %   ");
  Serial.println("-----+-------+-----------+------------+--------+--------");
}

static void printSummary() {
  Serial.println();
  Serial.println("=================== ЗВЕДЕННЯ ===================");
  Serial.printf("Вимірювань:            %u\n", samples);
  if (samples == 0) { Serial.println("================================================"); return; }
  Serial.printf("Середня похибка:       %.2f %%\n", errSum / samples);
  Serial.printf("Максимальна похибка:   %.2f %% (при RAW = %u)\n", errMax, errMaxRaw);
  Serial.println();
  Serial.println("  Діапазон RAW   | Замірів | Середня похибка");
  Serial.println("  ---------------+---------+----------------");
  for (uint8_t b = 0; b < BINS; b++) {
    if (binCount[b] == 0) continue;
    Serial.printf("  %4u .. %4u   |  %5u  |     %5.2f %%\n",
                  (unsigned)(b * 512), (unsigned)(b * 512 + 511),
                  binCount[b], binErrSum[b] / binCount[b]);
  }
  Serial.println("================================================");
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("Вимірювання напруги з дільника LDR, ESP32-S3");
  Serial.printf("Uref = %u мВ, ADCmax = %u, пін GPIO%u (ADC1_CH3)\n",
                U_REF_MV, ADC_MAX, LDR_PIN);
  Serial.println("Команда 's' + Enter -> зведення і розподіл похибки.");
  printHeader();
}

void loop() {
  static uint32_t lastRead = 0;
  static uint32_t rowCount = 0;

  if (Serial.available()) {
    const char c = (char)Serial.read();
    bool isCharC = c == 's' || c == 'S';
    if (isCharC) { 
        printSummary(); 
        printHeader(); 
    }
  }

  if (millis() - lastRead < PERIOD_MS) return;
  lastRead = millis();

  uint16_t raw = 0, measuredMv = 0;
  readAveraged(raw, measuredMv);

  const uint16_t calcMv = (uint16_t)(((uint32_t)raw * U_REF_MV) / ADC_MAX);

  const int16_t deltaMv = (int16_t)calcMv - (int16_t)measuredMv;

  float errPct = 0.0f;
  if (measuredMv > 0) {
    errPct = fabsf((float)deltaMv) * 100.0f / (float)measuredMv;

    samples++;
    errSum += errPct;
    if (errPct > errMax) { 
        errMax = errPct; errMaxRaw = raw; 
    }

    const uint8_t bin = raw / 512;
    if (bin < BINS) { 
        binErrSum[bin] += errPct; binCount[bin]++;
    }
  }

  if (++rowCount % 20 == 1 && rowCount > 1) {
    printHeader();
  }

  Serial.printf("%4u | %5u |   %5u   |    %5u   | %+5d  |  %5.2f\n",
                rowCount, raw, calcMv, measuredMv, deltaMv, errPct);
}

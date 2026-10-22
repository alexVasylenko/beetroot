#include <Arduino.h>

#define BUTTON_IN 16

constexpr uint32_t QUIET_US  = 50000;
constexpr size_t   MAX_EDGES = 256;

struct Edge {
  uint32_t t;     
  uint8_t  level;
};

volatile Edge     edges[MAX_EDGES];
volatile size_t   edgeCount  = 0;
volatile uint32_t lastEdgeUs = 0;
volatile uint32_t isrCalls   = 0;
volatile uint32_t lostEdges  = 0;


uint32_t pressNumber    = 0;
uint32_t sumDurationUs  = 0;
uint32_t maxDurationUs  = 0;
uint32_t maxEdgesInPress = 0;
uint32_t minGapUsGlobal = UINT32_MAX;

void IRAM_ATTR buttonIsr() {
  const uint32_t now = micros();
  isrCalls++;
  if (edgeCount < MAX_EDGES) {
    edges[edgeCount].t     = now;
    edges[edgeCount].level = (uint8_t)digitalRead(BUTTON_IN);
    edgeCount++;
  } else {
    lostEdges++;
  }
  lastEdgeUs = now;
}

static void printSummary() {
  Serial.println();
  Serial.println("================ ЗВЕДЕННЯ ================");
  Serial.printf("Натискань виміряно:        %u\n", pressNumber);
  if (pressNumber == 0) { 
    Serial.println("=========================================="); 
    return; 
  }
  Serial.printf("Середня тривалість брязкоту: %.3f мс\n",
                (sumDurationUs / (float)pressNumber) / 1000.0f);
  Serial.printf("Максимальна тривалість:      %.3f мс\n", maxDurationUs / 1000.0f);
  Serial.printf("Максимум фронтів за натиск:  %u\n", maxEdgesInPress);
  if (minGapUsGlobal != UINT32_MAX) {
    Serial.printf("Найкоротший інтервал:        %u мкс\n", minGapUsGlobal);
  }
  
  uint32_t rec = (uint32_t)((maxDurationUs / 1000.0f) * 2.0f + 0.999f);
  rec = ((rec + 4) / 5) * 5;
  if (rec < 10) {
    rec = 10;
  }
  Serial.printf("Достатній debounce:          %u мс (2x від максимуму, округлено)\n", rec);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON_IN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_IN), buttonIsr, CHANGE);

  Serial.println("Вимірювач брязкоту. Тисни кнопку 10 разів по-різному:");
  Serial.println("  швидко / повільно / сильно / ледь-ледь / під кутом");
  Serial.println("Команда 's' + Enter -> зведення по всіх натисканнях.");
  Serial.printf("Рівень на піні зараз: %d (має бути 1 — підтяжка працює)\n",
                digitalRead(BUTTON_IN));
}

void loop() {
  if (edgeCount > 0 && (micros() - lastEdgeUs) > QUIET_US) {

    noInterrupts();
    const size_t n = edgeCount;
    static Edge snap[MAX_EDGES];
    for (size_t i = 0; i < n; i++) {
      snap[i].t     = edges[i].t;
      snap[i].level = edges[i].level;
    }
    const uint32_t calls = isrCalls;
    const uint32_t lost  = lostEdges;
    edgeCount = 0;
    isrCalls  = 0;
    lostEdges = 0;
    interrupts();

    pressNumber++;

    const uint32_t durationUs = snap[n - 1].t - snap[0].t;
    uint32_t minGap = UINT32_MAX, maxGap = 0;
    uint32_t falling = 0, rising = 0;
    for (size_t i = 0; i < n; i++) {
      if (snap[i].level == 0) {
        falling++; 
      } else {
        rising++;
      }

      if (i > 0) {
        const uint32_t gap = snap[i].t - snap[i - 1].t;
        if (gap < minGap) minGap = gap;
        if (gap > maxGap) maxGap = gap;
      }
    }

    sumDurationUs += durationUs;
    if (durationUs > maxDurationUs) {
      maxDurationUs = durationUs;
    }
    if (n > maxEdgesInPress) {
      maxEdgesInPress = n;
    }
    if (n > 1 && minGap < minGapUsGlobal) {
        minGapUsGlobal = minGap;
    }
    Serial.println();
    Serial.printf("--- Натискання #%u ---\n", pressNumber);
    Serial.printf("Фронтів усього (CHANGE):   %u\n", (uint32_t)n);
    Serial.printf("  з них спадаючих (1->0):  %u   <- стільки побачив би код з FALLING\n", falling);
    Serial.printf("  з них зростаючих (0->1): %u\n", rising);
    Serial.printf("Викликів ISR:              %u", calls);
    Serial.println(calls == n + lost ? "  (збігається з фронтами)" : "  <- УВАГА: не збігається");
    if (lost) {
        Serial.printf("Не вмістилось у буфер:     %u\n", lost);
    }
    Serial.printf("Тривалість брязкоту:       %.3f мс\n", durationUs / 1000.0f);
    if (n > 1) {
      Serial.printf("Інтервали між фронтами:    min %u мкс / max %u мкс\n", minGap, maxGap);
    } else {
      Serial.println("Інтервали:                 брязкоту не було, один чистий фронт");
    }

    Serial.print("Часи фронтів від першого, мкс: ");
    for (size_t i = 0; i < n; i++) {
      Serial.printf("%u%s", snap[i].t - snap[0].t, (i + 1 < n) ? ", " : "\n");
    }
  }

  if (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == 's' || c == 'S') {
        printSummary();
    }
  }
}

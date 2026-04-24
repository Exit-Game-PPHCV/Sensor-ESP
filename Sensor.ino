// ===== PIN KONFIGURATION =====
const int SENSOR_PINS[3] = {4,5,6};
const int LED_PINS[3]    = {20,21,22};

const int SENSOR_COUNT = 3;

// ===== ADC LIMITS =====
const int ADC_MIN = 0;
const int ADC_MAX = 4095;

// ===== SPIEL PARAMETER =====
const int TARGET_WIDTH = 200;
const unsigned long ROUND_TIME_MS = 30000;

// ===== ZIELWERTE =====
int targetMin[SENSOR_COUNT];
int targetMax[SENSOR_COUNT];

// ===== ZEIT =====
unsigned long roundStartTime = 0;

// ===== GLOBALER STATUS =====
bool puzzleSolved = false;


// ===== ZIELBEREICHE GENERIEREN =====
void generateTargets() {
  for (int i = 0; i < SENSOR_COUNT; i++) {
    targetMin[i] = random(ADC_MIN, ADC_MAX - TARGET_WIDTH);
    targetMax[i] = targetMin[i] + TARGET_WIDTH;
  }

  roundStartTime = millis();
  puzzleSolved = false;

  Serial.println("=== Neue Zielbereiche ===");
  for (int i = 0; i < SENSOR_COUNT; i++) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(targetMin[i]);
    Serial.print(" - ");
    Serial.println(targetMax[i]);
  }
}


// ===== CHECK =====
bool inTarget(int i, int value) {
  return value >= targetMin[i] && value <= targetMax[i];
}


// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  randomSeed(analogRead(SENSOR_PINS[0]));

  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // LEDs aus
  }

  generateTargets();
}


// ===== LOOP =====
void loop() {

  // neue Runde nach Zeit
  if (millis() - roundStartTime >= ROUND_TIME_MS) {
    generateTargets();
  }

  bool allCorrect = true;

  for (int i = 0; i < SENSOR_COUNT; i++) {

    int value = analogRead(SENSOR_PINS[i]);

    if (inTarget(i, value)) {
      // LED AN wenn richtig
      digitalWrite(LED_PINS[i], HIGH);
    } else {
      // LED AUS wenn falsch
      digitalWrite(LED_PINS[i], LOW);
      allCorrect = false;
    }

    // DEBUG
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(value);

    Serial.print(" Ziel: ");
    Serial.print(targetMin[i]);
    Serial.print("-");
    Serial.print(targetMax[i]);

    Serial.print(inTarget(i, value) ? " OK" : " --");
    Serial.print(" | ");
  }

  // globaler Status
  puzzleSolved = allCorrect;

  Serial.print("Zeit: ");
  Serial.print((ROUND_TIME_MS - (millis() - roundStartTime)) / 1000);
  Serial.print("s");

  Serial.print(" || puzzleSolved: ");
  Serial.print(puzzleSolved ? "true" : "false");

  if (puzzleSolved) {
    Serial.print(" || ALLE RICHTIG");
  }

  Serial.println();

  delay(100);
}
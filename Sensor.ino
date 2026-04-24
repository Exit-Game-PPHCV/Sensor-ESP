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

// Erfolg-Phasen
const unsigned long BLINK_TIME_MS = 5000;      // 5 Sekunden blinken
const unsigned long HOLD_TIME_MS  = 120000;    // 2 Minuten an

// ===== ZIELWERTE =====
int targetMin[SENSOR_COUNT];
int targetMax[SENSOR_COUNT];

// ===== ZEIT =====
unsigned long roundStartTime = 0;
unsigned long successStartTime = 0;
unsigned long lastBlinkTime = 0;

// ===== STATUS =====
bool puzzleSolved = false;
bool blinkState = false;

// ===== MODES =====
enum GameState {
  PLAYING,
  SUCCESS_BLINK,
  SUCCESS_HOLD
};

GameState currentState = PLAYING;


// ===== ZIELBEREICHE =====
void generateTargets() {
  for (int i = 0; i < SENSOR_COUNT; i++) {
    targetMin[i] = random(ADC_MIN, ADC_MAX - TARGET_WIDTH);
    targetMax[i] = targetMin[i] + TARGET_WIDTH;
  }

  roundStartTime = millis();
  puzzleSolved = false;
  currentState = PLAYING;

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
    digitalWrite(LED_PINS[i], LOW);
  }

  generateTargets();
}


// ===== LOOP =====
void loop() {

  // ===== STATE: SPIEL LÄUFT =====
  if (currentState == PLAYING) {

    // Zeit abgelaufen → neue Runde
    if (millis() - roundStartTime >= ROUND_TIME_MS) {
      generateTargets();
    }

    bool allCorrect = true;

    for (int i = 0; i < SENSOR_COUNT; i++) {

      int value = analogRead(SENSOR_PINS[i]);

      if (inTarget(i, value)) {
        digitalWrite(LED_PINS[i], HIGH);
      } else {
        digitalWrite(LED_PINS[i], LOW);
        allCorrect = false;
      }

      // Debug
      Serial.print("S");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(value);
      Serial.print(" | ");
    }

    puzzleSolved = allCorrect;

    // Wenn alle richtig → Erfolg starten
    if (puzzleSolved) {
      Serial.println("!!! ALLE RICHTIG !!!");

      currentState = SUCCESS_BLINK;
      successStartTime = millis();
      lastBlinkTime = millis();
    }

    Serial.println();
  }


  // ===== STATE: BLINKEN =====
  else if (currentState == SUCCESS_BLINK) {

    // Blink alle 500ms
    if (millis() - lastBlinkTime >= 500) {
      lastBlinkTime = millis();
      blinkState = !blinkState;

      for (int i = 0; i < SENSOR_COUNT; i++) {
        digitalWrite(LED_PINS[i], blinkState ? HIGH : LOW);
      }
    }

    // Nach Blink-Zeit → Dauerlicht
    if (millis() - successStartTime >= BLINK_TIME_MS) {
      currentState = SUCCESS_HOLD;

      // Alle LEDs AN
      for (int i = 0; i < SENSOR_COUNT; i++) {
        digitalWrite(LED_PINS[i], HIGH);
      }

      successStartTime = millis();
    }
  }


  // ===== STATE: 2 MINUTEN AN =====
  else if (currentState == SUCCESS_HOLD) {

    // Nach 2 Minuten → neue Runde
    if (millis() - successStartTime >= HOLD_TIME_MS) {
      generateTargets();
    }
  }

  delay(50);
}
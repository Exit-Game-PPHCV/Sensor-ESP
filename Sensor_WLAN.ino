#include <Zigbee.h>

// =======================================================
// ===== ZIGBEE STECKDOSEN-ENDPUNKTE FÜR BOOLEAN-WERTE ====
// =======================================================
//
// Jeder Boolean-Status wird als eigene Zigbee-Steckdose registriert:
//
// EP 1: gyro/poti minigame gelöst
// EP 2: laser/ldr minigame gelöst
// EP 3: gesamtes Modul gelöst
//
// false = Steckdose AUS
// true  = Steckdose AN

#define ZB_ENDPOINT_PUZZLE_SOLVED 1
#define ZB_ENDPOINT_LDR_SOLVED    2
#define ZB_ENDPOINT_MODULE_SOLVED 3

ZigbeePowerOutlet zbPuzzleSolved = ZigbeePowerOutlet(ZB_ENDPOINT_PUZZLE_SOLVED);
ZigbeePowerOutlet zbLdrSolved    = ZigbeePowerOutlet(ZB_ENDPOINT_LDR_SOLVED);
ZigbeePowerOutlet zbModuleSolved = ZigbeePowerOutlet(ZB_ENDPOINT_MODULE_SOLVED);


// ===== ZIGBEE STATUS CACHE =====
// Damit nicht unnötig dauerhaft gesendet wird
bool lastSentPuzzleSolved = false;
bool lastSentLdrSolved = false;
bool lastSentModuleSolved = false;
bool firstZigbeeSend = true;


// ===== PIN KONFIGURATION =====
const int SENSOR_PINS[3] = {4, 5, 0};
const int LED_PINS[3]    = {20, 21, 22};

// LDR / Fotosensor
const int LDR_PIN = 6;        // ADC-Pin für LDR anpassen
const int LDR_LED_PIN = 23;   // LED für LDR anpassen

const int SENSOR_COUNT = 3;

// ===== ADC LIMITS =====
const int ADC_MIN = 0;
const int ADC_MAX = 4095;

// ===== LDR PARAMETER =====
// Wenn der LDR-Wert größer als dieser Wert ist, gilt: Laser/Licht trifft Sensor
// Diesen Wert musst du später im Serial Monitor fein einstellen.
const int LDR_THRESHOLD = 3750;

// ===== SPIEL PARAMETER =====
const int TARGET_WIDTH = 200;
const unsigned long ROUND_TIME_MS = 30000;

// Erfolg-Phasen
const unsigned long BLINK_TIME_MS = 5000;
const unsigned long HOLD_TIME_MS  = 120000;

// ===== ZIELWERTE =====
int targetMin[SENSOR_COUNT];
int targetMax[SENSOR_COUNT];

// ===== ZEIT =====
unsigned long roundStartTime = 0;
unsigned long successStartTime = 0;
unsigned long lastBlinkTime = 0;

// ===== STATUS =====
bool puzzleSolved = false;
bool ldrSolved = false;
bool blinkState = false;

// Gesamtstatus für beide Minispiele
bool moduleSolved = false;

// ===== MODES =====
enum GameState {
  PLAYING,
  SUCCESS_BLINK,
  SUCCESS_HOLD
};

GameState currentState = PLAYING;


// =======================================================
// ===== ZIGBEE SETUP ====================================
// =======================================================

void setupZigbee() {
  Serial.println("Starte Zigbee...");

  // Namen/Modelle setzen, damit sie auf dem Raspberry Pi/Zigbee2MQTT/Home Assistant
  // besser unterscheidbar sind.
  zbPuzzleSolved.setManufacturerAndModel("EscapeGame", "PuzzleSolved");
  zbLdrSolved.setManufacturerAndModel("EscapeGame", "LdrSolved");
  zbModuleSolved.setManufacturerAndModel("EscapeGame", "ModuleSolved");

  // Jeden Boolean als eigene Steckdose registrieren
  Serial.println("Registriere Zigbee-Steckdose: puzzleSolved");
  Zigbee.addEndpoint(&zbPuzzleSolved);

  Serial.println("Registriere Zigbee-Steckdose: ldrSolved");
  Zigbee.addEndpoint(&zbLdrSolved);

  Serial.println("Registriere Zigbee-Steckdose: moduleSolved");
  Zigbee.addEndpoint(&zbModuleSolved);

  // Zigbee starten
  // PowerOutlet-Beispiele nutzen ZIGBEE_ROUTER.
  if (!Zigbee.begin(ZIGBEE_ROUTER)) {
    Serial.println("Zigbee konnte nicht gestartet werden!");
    Serial.println("ESP wird neu gestartet...");
    ESP.restart();
  }

  Serial.println("Zigbee gestartet. Warte auf Verbindung zum Netzwerk...");

  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }

  Serial.println();
  Serial.println("Zigbee verbunden!");
}


// =======================================================
// ===== ZIGBEE SENDEN ===================================
// =======================================================

void sendBooleanOutletsToZigbee() {
  moduleSolved = puzzleSolved && ldrSolved;

  bool changed =
    firstZigbeeSend ||
    puzzleSolved != lastSentPuzzleSolved ||
    ldrSolved != lastSentLdrSolved ||
    moduleSolved != lastSentModuleSolved;

  if (!changed) {
    return;
  }

  Serial.println("=== Sende Zigbee-Status ===");

  Serial.print("puzzleSolved Steckdose: ");
  Serial.println(puzzleSolved ? "AN" : "AUS");

  Serial.print("ldrSolved Steckdose: ");
  Serial.println(ldrSolved ? "AN" : "AUS");

  Serial.print("moduleSolved Steckdose: ");
  Serial.println(moduleSolved ? "AN" : "AUS");

  // Jeder Boolean wird als eigene Steckdose gesetzt.
  // false = aus, true = an
  zbPuzzleSolved.setState(puzzleSolved);
  zbLdrSolved.setState(ldrSolved);
  zbModuleSolved.setState(moduleSolved);

  lastSentPuzzleSolved = puzzleSolved;
  lastSentLdrSolved = ldrSolved;
  lastSentModuleSolved = moduleSolved;
  firstZigbeeSend = false;
}


// ===== ZIELBEREICHE =====
void generateTargets() {
  for (int i = 0; i < SENSOR_COUNT; i++) {
    targetMin[i] = random(ADC_MIN, ADC_MAX - TARGET_WIDTH);
    targetMax[i] = targetMin[i] + TARGET_WIDTH;
  }

  roundStartTime = millis();
  puzzleSolved = false;
  blinkState = false;
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


// ===== LDR LESEN =====
void updateLdr() {
  int ldrValue = analogRead(LDR_PIN);

  // Je nach Verdrahtung kann es sein, dass bei Licht der Wert hoch ODER niedrig wird.
  // Bei deiner aktuellen Logik: hoher Wert = Licht/Laser trifft.

  if (ldrValue >= LDR_THRESHOLD) {
    ldrSolved = true;
  }

  if (ldrSolved) {
    digitalWrite(LDR_LED_PIN, HIGH);
  } else {
    digitalWrite(LDR_LED_PIN, LOW);
  }

  Serial.print("LDR: ");
  Serial.print(ldrValue);
  Serial.print(ldrSolved ? " OK" : " --");
  Serial.print(" | ");
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

  pinMode(LDR_LED_PIN, OUTPUT);
  digitalWrite(LDR_LED_PIN, LOW);

  setupZigbee();

  generateTargets();

  // Anfangszustand einmal an Zigbee senden
  sendBooleanOutletsToZigbee();
}


// ===== LOOP =====
void loop() {

  // LDR soll immer geprüft werden
  updateLdr();

  // ===== STATE: SPIEL LÄUFT =====
  if (currentState == PLAYING) {

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

      Serial.print("S");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(value);
      Serial.print(" | ");
    }

    puzzleSolved = allCorrect;

    if (puzzleSolved) {
      Serial.println("!!! ALLE 3 POTIS RICHTIG !!!");

      currentState = SUCCESS_BLINK;
      successStartTime = millis();
      lastBlinkTime = millis();
    }

    Serial.println();
  }


  // ===== STATE: BLINKEN =====
  else if (currentState == SUCCESS_BLINK) {

    if (millis() - lastBlinkTime >= 500) {
      lastBlinkTime = millis();
      blinkState = !blinkState;

      for (int i = 0; i < SENSOR_COUNT; i++) {
        digitalWrite(LED_PINS[i], blinkState ? HIGH : LOW);
      }
    }

    if (millis() - successStartTime >= BLINK_TIME_MS) {
      currentState = SUCCESS_HOLD;

      for (int i = 0; i < SENSOR_COUNT; i++) {
        digitalWrite(LED_PINS[i], HIGH);
      }

      successStartTime = millis();
    }

    Serial.println();
  }


  // ===== STATE: 2 MINUTEN AN =====
  else if (currentState == SUCCESS_HOLD) {

    if (millis() - successStartTime >= HOLD_TIME_MS) {
      generateTargets();
    }

    Serial.println();
  }

  // Nach jeder Spiellogik prüfen:
  // Haben sich Boolean-Zustände geändert?
  // Falls ja: jeweilige Zigbee-Steckdose aktualisieren.
  sendBooleanOutletsToZigbee();

  delay(50);
}

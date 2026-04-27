#include <Keypad.h>

// ===== KEYPAD SETUP =====
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Deine 8 Keypad-Pins:
// 3, 2, 11, 10, 8, 1, 0, 7
//
// Annahme:
// erste 4 Kabel = Reihen
// letzte 4 Kabel = Spalten
byte rowPins[ROWS] = {3, 2, 11, 10};
byte colPins[COLS] = {8, 1, 0, 7};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===== LED PINS =====
const int RED_LED_PIN = 21;    // LED 1 rot
const int GREEN_LED_PIN = 20;  // LED 2 grün

// ===== CODE =====
String inputCode = "";
const String correctCode = "121950";

// später für Flask / Server
bool puzzleSolved = false;

// ===== PRÜF-ANIMATION =====
const unsigned long CHECK_TIME_MS = 1000;
const unsigned long BLINK_INTERVAL_MS = 100;

void setStatusLeds() {
  if (puzzleSolved) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  } else {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  }
}

void checkAnimation() {
  unsigned long startTime = millis();
  bool ledState = false;

  while (millis() - startTime < CHECK_TIME_MS) {
    ledState = !ledState;

    digitalWrite(RED_LED_PIN, ledState ? HIGH : LOW);
    digitalWrite(GREEN_LED_PIN, ledState ? HIGH : LOW);

    delay(BLINK_INTERVAL_MS);
  }
}

void checkInputCode() {
  Serial.print("Pruefe Eingabe: ");
  Serial.println(inputCode);

  checkAnimation();

  if (inputCode == correctCode) {
    puzzleSolved = true;
    Serial.println("RICHTIG -> puzzleSolved = true");
  } else {
    puzzleSolved = false;
    Serial.println("FALSCH -> puzzleSolved = false");
  }

  inputCode = "";
  setStatusLeds();

  Serial.print("Aktueller Boolean fuer Server: ");
  Serial.println(puzzleSolved ? "true" : "false");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  puzzleSolved = false;
  setStatusLeds();

  Serial.println("Keypad bereit.");
  Serial.println("Code eingeben und mit * pruefen.");
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Taste: ");
    Serial.println(key);

    if (key == '*') {
      checkInputCode();
      return;
    }

    if (key == '#') {
      inputCode = "";
      Serial.println("Eingabe geloescht");
      return;
    }

  if (key >= '0' && key <= '9') {
    if (inputCode.length() < correctCode.length()) {
     inputCode += key;
    }

      Serial.print("Aktuelle Eingabe: ");
      Serial.println(inputCode);
    }
  }
}
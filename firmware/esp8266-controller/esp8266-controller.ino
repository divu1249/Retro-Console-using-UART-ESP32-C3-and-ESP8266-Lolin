#define BTN_UP     4
#define BTN_DOWN   13
#define BTN_LEFT   0
#define BTN_RIGHT  2
#define BTN_A      15
#define BTN_B      16
#define BUZZER_PIN 14
#define BATT_PIN   A0

const int pins[6]     = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B};
const char* names[6]  = {"UP", "DOWN", "LEFT", "RIGHT", "A", "B"};
bool activeHigh[6]    = {false, false, false, false, true, false};

bool lastStable[6]    = {false, false, false, false, false, false};
bool lastReading[6]   = {false, false, false, false, false, false};
unsigned long lastChange[6] = {0};
const unsigned long DEBOUNCE = 25;

String rxBuffer = "";

unsigned long lastBattCheck = 0;
const unsigned long BATT_CHECK_INTERVAL = 5000;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 6; i++) {
    pinMode(pins[i], (pins[i] == 16) ? INPUT : INPUT_PULLUP);
  }
  pinMode(BUZZER_PIN, OUTPUT);
  lastBattCheck = millis() - BATT_CHECK_INTERVAL;
}

void loop() {
  // Button scanning
  for (int i = 0; i < 6; i++) {
    bool raw = digitalRead(pins[i]);
    bool pressedRaw = activeHigh[i] ? (raw == HIGH) : (raw == LOW);

    if (pressedRaw != lastReading[i]) {
      lastChange[i] = millis();
      lastReading[i] = pressedRaw;
    }

    if ((millis() - lastChange[i]) > DEBOUNCE && pressedRaw != lastStable[i]) {
      lastStable[i] = pressedRaw;
      Serial.print("B:");
      Serial.print(names[i]);
      Serial.print(":");
      Serial.println(pressedRaw ? "1" : "0");
    }
  }

  // Battery check, periodic
  if (millis() - lastBattCheck > BATT_CHECK_INTERVAL) {
    lastBattCheck = millis();
    int raw = analogRead(BATT_PIN);
    float voltageAtPin = (raw / 1023.0) * 3.3;
    float batteryVoltage = voltageAtPin * 2.0;
    int pct = batteryPercent(batteryVoltage);

    Serial.print("V:");
    Serial.println(pct);
  }

  // Listen for commands from C3
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handleCommand(rxBuffer);
      rxBuffer = "";
    } else {
      rxBuffer += c;
    }
  }
}

int batteryPercent(float v) {
  if (v >= 4.2) return 100;
  if (v <= 3.0) return 0;
  return (int)((v - 3.0) / (4.2 - 3.0) * 100.0);
}

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.startsWith("S:")) {
    int sep = cmd.indexOf(':', 2);
    int freq = cmd.substring(2, sep).toInt();
    int ms = cmd.substring(sep + 1).toInt();
    tone(BUZZER_PIN, freq, ms);
  }
}
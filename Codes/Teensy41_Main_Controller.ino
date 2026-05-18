// ================= ADS1232 =================

const int DOUT_PIN = 8;
const int SCLK_PIN = 9;
const int PDWN_PIN = 10;

const unsigned int SCLK_US = 8;

// ================= HARDWARE =================

const int START_BUTTON = 2;
const int ESTOP_BUTTON = 3;

const int RELAY_SAND = 29;
const int RELAY_CEMENT = 30;

const int PUMP_WATER = 27;

const int HOPPER_MOTOR = 4;

const int BUZZER_PIN = 18;

// ================= UART =================

HardwareSerial &ESPSerial = Serial1;

// ================= VARIABLES =================

long offsetCounts = 0;

// CHANGE THIS ACCORDING TO CALIBRATION
float scale_counts_per_kg = 10000.0;

float liveWeight = 0;

bool systemReady = false;

bool processRunning = false;

volatile bool emergencyFlag = false;

volatile bool remoteStart = false;

unsigned long lastWeightUpdate = 0;

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  ESPSerial.begin(115200);

  pinMode(DOUT_PIN, INPUT);

  pinMode(SCLK_PIN, OUTPUT);

  pinMode(PDWN_PIN, OUTPUT);

  digitalWrite(PDWN_PIN, HIGH);

  pinMode(START_BUTTON, INPUT_PULLUP);

  pinMode(ESTOP_BUTTON, INPUT_PULLUP);

  pinMode(RELAY_SAND, OUTPUT);

  pinMode(RELAY_CEMENT, OUTPUT);

  pinMode(PUMP_WATER, OUTPUT);

  pinMode(HOPPER_MOTOR, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  attachInterrupt(
    digitalPinToInterrupt(ESTOP_BUTTON),
    emergencyStop,
    FALLING
  );

  allOff();

  // ==========================================
  // CALIBRATION
  // ==========================================

  Serial.println("================================");
  Serial.println("LOAD CELL IS CALIBRATING");
  Serial.println("WAIT...");
  Serial.println("================================");

  delay(5000);

  offsetCounts = readAverageRaw(20);

  Serial.println("================================");
  Serial.println("LOAD CELL CALIBRATED");
  Serial.println("SYSTEM READY");
  Serial.println("================================");

  systemReady = true;
}

// ======================================================
// EMERGENCY
// ======================================================

void emergencyStop() {

  emergencyFlag = true;

  allOff();
}

// ======================================================
// UPDATE WEIGHT
// ======================================================

void updateWeight() {

  if (millis() - lastWeightUpdate >= 200) {

    lastWeightUpdate = millis();

    long raw = readAverageRaw(3);

    liveWeight =
      abs((float)(raw - offsetCounts))
      /
      scale_counts_per_kg;
  }
}

// ======================================================
// SEND DATA TO ESP32
// ======================================================

void sendDataToESP(String stage) {

  String data = "";

  data += "STAGE=" + stage;

  data += ",WEIGHT=" + String(liveWeight, 2);

  data += ",SAND=" + String(digitalRead(RELAY_SAND));

  data += ",CEMENT=" + String(digitalRead(RELAY_CEMENT));

  data += ",WATER=" + String(digitalRead(PUMP_WATER));

  data += ",HOPPER=" + String(digitalRead(HOPPER_MOTOR));

  data += ",EMERGENCY=" + String(emergencyFlag);

  ESPSerial.println(data);
}

// ======================================================
// SERIAL MONITOR
// ======================================================

void printStatus(String stage) {

  Serial.println("================================");

  Serial.print("STAGE       : ");
  Serial.println(stage);

  Serial.print("WEIGHT      : ");
  Serial.print(liveWeight);
  Serial.println(" kg");

  Serial.print("SAND RELAY  : ");
  Serial.println(digitalRead(RELAY_SAND));

  Serial.print("CEMENT RELAY: ");
  Serial.println(digitalRead(RELAY_CEMENT));

  Serial.print("WATER PUMP  : ");
  Serial.println(digitalRead(PUMP_WATER));

  Serial.print("HOPPER      : ");
  Serial.println(digitalRead(HOPPER_MOTOR));

  Serial.print("EMERGENCY   : ");
  Serial.println(emergencyFlag);

  Serial.println("================================");
}

// ======================================================
// ESP COMMANDS
// ======================================================

void checkESPCommands() {

  while (ESPSerial.available()) {

    String cmd = ESPSerial.readStringUntil('\n');

    cmd.trim();

    if (cmd.indexOf("START") >= 0) {

      remoteStart = true;

      Serial.println("REMOTE START RECEIVED");
    }

    if (cmd.indexOf("STOP") >= 0) {

      emergencyFlag = true;

      allOff();

      Serial.println("REMOTE STOP RECEIVED");
    }
  }
}

// ======================================================
// ALL OFF
// ======================================================

void allOff() {

  digitalWrite(RELAY_SAND, LOW);

  digitalWrite(RELAY_CEMENT, LOW);

  digitalWrite(PUMP_WATER, LOW);

  digitalWrite(HOPPER_MOTOR, LOW);

  digitalWrite(BUZZER_PIN, LOW);
}

// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  updateWeight();

  checkESPCommands();

  sendDataToESP("READY");

  printStatus("READY");

  // ==========================================
  // EMERGENCY
  // ==========================================

  if (emergencyFlag) {

    sendDataToESP("EMERGENCY");

    while (1) {

      digitalWrite(BUZZER_PIN, HIGH);

      delay(100);

      digitalWrite(BUZZER_PIN, LOW);

      delay(100);
    }
  }

  // ==========================================
  // START CONDITION
  // ==========================================

  if (
      (digitalRead(START_BUTTON) == LOW || remoteStart)
      &&
      !processRunning
     ) {

    remoteStart = false;

    processRunning = true;

    Serial.println("PROCESS STARTED");

    // ======================================
    // BUZZER INDICATION
    // START PRESSED
    // ======================================

    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    // ======================================
    // STAGE 1 : SAND
    // ======================================

    digitalWrite(RELAY_SAND, HIGH);

    while (liveWeight < 2.0 && !emergencyFlag) {

      updateWeight();

      checkESPCommands();

      sendDataToESP("SAND");

      printStatus("SAND");

      delay(200);
    }

    // ======================================
    // RELAY 1 OFF
    // RELAY 2 ON
    // ======================================

    digitalWrite(RELAY_SAND, LOW);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    // ======================================
    // STAGE 2 : CEMENT
    // ======================================

    digitalWrite(RELAY_CEMENT, HIGH);

    while (liveWeight < 4.0 && !emergencyFlag) {

      updateWeight();

      checkESPCommands();

      sendDataToESP("CEMENT");

      printStatus("CEMENT");

      delay(200);
    }

    // ======================================
    // RELAY 2 OFF
    // PUMP ON
    // ======================================

    digitalWrite(RELAY_CEMENT, LOW);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    // ======================================
    // STAGE 3 : WATER PUMP
    // ======================================

    digitalWrite(PUMP_WATER, HIGH);

    unsigned long waterStart = millis();

    while (millis() - waterStart < 10000) {

      updateWeight();

      checkESPCommands();

      sendDataToESP("WATER");

      printStatus("WATER");

      delay(200);
    }

    // ======================================
    // PUMP OFF
    // ======================================

    digitalWrite(PUMP_WATER, LOW);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    // ======================================
    // STAGE 4 : HOPPER
    // ======================================

    digitalWrite(HOPPER_MOTOR, HIGH);

    unsigned long hopperStart = millis();

    while (millis() - hopperStart < 20000) {

      updateWeight();

      checkESPCommands();

      sendDataToESP("HOPPER");

      printStatus("HOPPER");

      delay(200);
    }

    digitalWrite(HOPPER_MOTOR, LOW);

    // ======================================
    // FINAL BUZZER
    // PROCESS COMPLETE
    // ======================================

    unsigned long buzzerStart = millis();

    while (millis() - buzzerStart < 10000) {

      updateWeight();

      checkESPCommands();

      sendDataToESP("PROCESS COMPLETED");

      printStatus("PROCESS COMPLETED");

      digitalWrite(BUZZER_PIN, HIGH);

    
    }

    processRunning = false;

    Serial.println("PROCESS FINISHED");
  }

  delay(200);
}
// ======================================================
// ADC READ
// ======================================================

long readRawADC() {

  while (digitalRead(DOUT_PIN) == HIGH);

  unsigned long data = 0;

  for (int i = 0; i < 24; i++) {

    digitalWrite(SCLK_PIN, HIGH);

    delayMicroseconds(SCLK_US);

    data = (data << 1) | digitalRead(DOUT_PIN);

    digitalWrite(SCLK_PIN, LOW);

    delayMicroseconds(SCLK_US);
  }

  digitalWrite(SCLK_PIN, HIGH);

  delayMicroseconds(SCLK_US);

  digitalWrite(SCLK_PIN, LOW);

  if (data & 0x800000UL) {

    data |= 0xFF000000UL;
  }

  return (long)data;
}

// ======================================================
// AVERAGE ADC
// ======================================================

long readAverageRaw(int samples) {

  long sum = 0;

  for (int i = 0; i < samples; i++) {

    sum += readRawADC();

    delay(10);
  }

  return sum / samples;
}
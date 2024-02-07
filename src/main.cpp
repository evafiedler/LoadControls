// #include <Adafruit_INA260.h>
#include <Arduino.h>
// Adafruit_INA260 ina260 = Adafruit_INA260();

enum STATES {
  initState,
  normalOp,
  eStop,
  loadDisconnect
};

// Pin for current sensor reading
const int currentSensor = A0;
const float VOLTAGE_REF = 5.0; // Volts
const float RESISTANCE = 30.0; // Ohms

const int relaySetSignal = 12;
const int relayResetSignal = 13;

STATES state;
int relayState; // 0 for load, 1 for PCC

float readCurrent();

void switchRelayToLoad();
void switchRelayToPCC();

void initialize();
void normalOperation();
void emergencyStop();
void loadDisconnection();

void setup() {
  Serial.begin(9600);

  while (!Serial) {delay(10);}
  
  //ina260.begin();

  pinMode (7, OUTPUT);
  pinMode(relaySetSignal, OUTPUT);
  pinMode(relayResetSignal, OUTPUT);

  state = initState;
  //switchRelayToLoad();
  //relayState = 1;
  delay(10);
}

void loop() {
  
  switch (state) {
    case initState:
      initialize();
      break;
    case normalOp:
      normalOperation();
      break;
    case eStop:
      emergencyStop();
      break;
    case loadDisconnect:
      loadDisconnection();
      break;
  }
}

// Helper function for reading current in Amps
float readCurrent() {
  float current = (analogRead(currentSensor) * VOLTAGE_REF) / 1023;
  return current;
}

void switchRelayToLoad() {      
  digitalWrite(relaySetSignal, HIGH);
  delay(5);
  digitalWrite(relaySetSignal, LOW);
  relayState = 0;
}

void switchRelayToPCC() {
  digitalWrite(relayResetSignal, HIGH);
  delay(5);
  digitalWrite(relayResetSignal, LOW);
  relayState = 1;
}

// TODO: check this logic (not immediately setting to PCC)
// I think this requires that load is always connected and turned on first
void initialize() {
  switchRelayToLoad();
  delay(10);
  float voltage = readCurrent() * RESISTANCE; // voltage in V

  if (voltage < 7) {
    // turbine side arduino not receiving enough power
    switchRelayToPCC();
    while (Serial.available() <= 0) {
      delay(10);
    }
    char c = Serial.read();
    if (c == 'O') {
      // Turbine side generating it's own power
      switchRelayToLoad();
    }
  }
  state = normalOp;
}

void normalOperation() {   
  digitalWrite(7, LOW); // TODO: what is this line for???

  // Power and voltage readings
  float current = readCurrent();
  float voltage = current * RESISTANCE; // voltage in V 
  float power = current * voltage; // power in W
  //Serial.println(power);

  char c;
  if (Serial.available() > 0) {
    c = Serial.read();

    if (c == 'E') {
      state = eStop;
      switchRelayToPCC();
    }
  }

  if (relayState == 0 && voltage < 0.1){ //arbitrarilly low threshold for load disconnected voltage (TODO: determine experimentally)
    state = loadDisconnect;
    Serial.print('D');
    switchRelayToPCC();
  } 
}

void emergencyStop() {
  //digitalWrite(7, HIGH);
  //Serial.println("ESTOP");
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == 'N') {
      digitalWrite(7, HIGH);
      state = normalOp;
      switchRelayToLoad();
    }
  }
}

// Pretty sure the logic for this is messed up -- ask Alyssa for her copies
// I think you just need to wait for 'O' (possible switch to 'N' as that now indicates moving into normal mode)
void loadDisconnection() {
  char c;
  if (Serial.available()) {
    c = Serial.read();
    if (c == 'O') {
      // TODO: in Turbine Controls I don't think we handle this case???
      Serial.print('R');
    }

    if (c == 'N') {
      state = normalOp;
      switchRelayToLoad();
    }
  }
}

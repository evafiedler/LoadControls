#include <Adafruit_INA260.h>

Adafruit_INA260 ina260 = Adafruit_INA260();

enum STATES {
  initState,
  normalOp,
  eStop,
  loadDisconnect
};

const int relaySetSignal = 12;
const int relayResetSignal = 13;

STATES state;
int relayState; // 0 for load, 1 for PCC

void switchRelayToLoad();
void switchRelayToPCC();

void initialize();
void normalOperation();
void emergencyStop();
void loadDisconection();

void setup() {
  Serial.begin(9600);

  while (!Serial) {delay(10);}
  
  ina260.begin();

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

void initialize() {
  switchRelayToLoad();
  delay(10);
  //ina260.begin();
  float voltage = ina260.readBusVoltage();

  if (voltage < 7000) {
    // turbine side arduino not receiving enough power
    switchRelayToPCC();
    while (Serial.available() <= 0) {
      delay(10);
    }
    char c = Serial.read();
    if (c == 'O') {
      switchRelayToLoad();
    }
  }
  state = normalOp;
}

void normalOperation() {
  digitalWrite(7, LOW);
  //ina260.begin();
    // Power and voltage readings
  float power = ina260.readPower(); //power in mW
  float voltage = ina260.readBusVoltage() / 1000; //voltage in V
  //Serial.println(power);
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    //Serial.println("char read");
    if (c == 'E') {
      state = eStop;
      switchRelayToPCC();
    }
  }

  if (relayState == 0 && voltage < 1){ //arbitrarilly low threshold for load disconnected voltage (determine experimentally)
    state = loadDisconnect;
    Serial.print('D');
    switchRelayToPCC();
  }

//  if (relayState == 1) {
//    switchRelayToLoad();
//  }
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

void loadDisconnection() {
  char c;
  if (Serial.available()) {
    c = Serial.read();
    if (c == 'O') {
      Serial.print('R');
    }

    if (c == 'N') {
      state = normalOp;
      switchRelayToLoad();
    }
  }
}

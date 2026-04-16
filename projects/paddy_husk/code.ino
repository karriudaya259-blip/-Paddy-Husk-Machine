#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

// ========== Pin Definitions (Matches your diagram.json) ==========
#define ONE_WIRE_BUS A0        // DS18B20 data pin
#define HEATER_LED_PIN 4       // Red LED (heater indicator)
#define READY_LED_PIN 6        // Green LED (plate ready)
#define SERVO_PIN 7            // Servo motor control
#define START_BTN_PIN 11       // START button (INPUT_PULLUP)
#define STOP_BTN_PIN 12        // STOP button (INPUT_PULLUP)
#define RESET_BTN_PIN 13       // RESET button (INPUT_PULLUP)

// ========== System Constants ==========
const float TARGET_TEMP = 120.0;        // Target temperature (°C)
const int PRESS_ANGLE = 90;             // Servo angle for pressing
const int RELEASE_ANGLE = 0;            // Servo angle for release
const unsigned long PRESS_HOLD_MS = 3000; // Hold pressure for 3 seconds

// ========== Global Objects ==========
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo pressServo;

// ========== State Machine ==========
enum State {
  IDLE,
  HEATING,
  PRESSING,
  PLATE_READY
};
State currentState = IDLE;

float currentTemp = 0.0;
unsigned long pressStartTime = 0;

void setup() {
  Serial.begin(9600);
  sensors.begin();
  pressServo.attach(SERVO_PIN);
  pressServo.write(RELEASE_ANGLE);     // Start with servo at release position

  pinMode(HEATER_LED_PIN, OUTPUT);
  pinMode(READY_LED_PIN, OUTPUT);
  pinMode(START_BTN_PIN, INPUT_PULLUP);
  pinMode(STOP_BTN_PIN, INPUT_PULLUP);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  digitalWrite(HEATER_LED_PIN, LOW);
  digitalWrite(READY_LED_PIN, LOW);

  Serial.println("System Ready. Press START.");
}

void loop() {
  // Read temperature
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0);

  // Button handling (with debounce delay)
  if (digitalRead(START_BTN_PIN) == LOW && currentState == IDLE) {
    currentState = HEATING;
    digitalWrite(HEATER_LED_PIN, HIGH);
    Serial.println("START pressed. Heating started.");
    delay(200);   // debounce
  }

  if (digitalRead(STOP_BTN_PIN) == LOW && currentState != IDLE) {
    emergencyStop();
    delay(200);
  }

  if (digitalRead(RESET_BTN_PIN) == LOW) {
    resetSystem();
    delay(200);
  }

  // State machine
  switch (currentState) {
    case HEATING:
      Serial.print("Heating... Temperature: ");
      Serial.print(currentTemp);
      Serial.println(" °C");
      
      if (currentTemp >= TARGET_TEMP) {
        digitalWrite(HEATER_LED_PIN, LOW);
        currentState = PRESSING;
        pressStartTime = millis();
        pressServo.write(PRESS_ANGLE);
        Serial.println("Target temperature reached. Pressing...");
      }
      break;

    case PRESSING:
      if (millis() - pressStartTime >= PRESS_HOLD_MS) {
        pressServo.write(RELEASE_ANGLE);
        currentState = PLATE_READY;
        digitalWrite(READY_LED_PIN, HIGH);
        Serial.println("Pressing complete. Plate ready!");
      }
      break;

    case PLATE_READY:
      delay(2000);
      digitalWrite(READY_LED_PIN, LOW);
      currentState = IDLE;
      Serial.println("System reset to IDLE. Ready for next cycle.");
      break;

    case IDLE:
      // Do nothing, wait for START
      break;
  }

  delay(100);
}

void emergencyStop() {
  digitalWrite(HEATER_LED_PIN, LOW);
  digitalWrite(READY_LED_PIN, LOW);
  pressServo.write(RELEASE_ANGLE);
  currentState = IDLE;
  Serial.println("EMERGENCY STOP: System halted.");
}

void resetSystem() {
  emergencyStop();
  Serial.println("System reset to IDLE.");
}

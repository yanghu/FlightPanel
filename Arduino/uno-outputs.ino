#include <Servo.h>
#define BAUD_RATE 9600

#define TRIM_SERVO_PIN 6
#define FLAP_SERVO_PIN 7
// Servo motor range. Need to calibrate before using.
#define TRIM_MAX_ANGLE 113
#define TRIM_MIN_ANGLE 57
#define FLAP_MAX_ANGLE 113
#define FLAP_MIN_ANGLE 64

#define PARKING_LED_PIN 2
#define GEAR_LED_PIN 3


enum LedState {
  LED_OFF,
  LED_ON,
  LED_BLINK
};


struct SimData{
  // -100 to 100, -100 means nose down.
  char trimPos;
  // Total number of flap positions
  char flapCnt;
  // Flap position index. [0, flapCnt]
  char flapPos;
  // 0 to 100. 100 is fully extended.
  char landingGearPos;
  bool parkingBrakeOn;
};



struct InstrumentStatus {
  char trimServoAngle;
  char flapServoAngle;
  bool parkingBrakeLight;
  bool landingGearLight;
};


int bytesRead = 0;
SimData simData;
InstrumentStatus panel;

int bufSize = sizeof(SimData);
Servo trimServo;
Servo flapServo;
char readBuf[10];
char outputStr[100];

class Led{
 public:
  Led(uint8_t pin, int blink_period);
  void Setup();
  void Update(LedState state);

 private:
  void Write(bool on);
  int blink_period_ = 1000;
  uint8_t pin_;
  int blink_counter_;
  LedState state_;
  // If the led is currently on
  bool led_on_;
};

Led::Led(uint8_t pin, int blink_period = 1000) {
  pin_ = pin;
  state_ = LED_OFF;
  led_on_ = false;
  blink_counter_ = 0;
  blink_period_ = blink_period;
}

void Led::Setup() {
  pinMode(pin_, OUTPUT);
}

void Led::Update(LedState state) {
  if (state != state_) {
    state_ = state;
    // Reset blink counter if blink started.
    if (state == LED_BLINK) {
      blink_counter_ = 0;
    }
  }
  // Update the led.
  switch (state_) {
    case LED_OFF:
      Write(LOW);
      break;
    case LED_ON:
      Write(HIGH);
      break;
    case LED_BLINK:
      blink_counter_++;
      blink_counter_ %= blink_period_;
      if (blink_counter_ < blink_period_/2) {
        Write(HIGH);
      } else {
        Write(LOW);
      }
  }
}

// Write digital pin if state change is needed.
void Led::Write(bool led_on) {
  if (led_on_ != led_on) {
    led_on_ = led_on;
    digitalWrite(pin_, led_on? HIGH: LOW);
  }
}

// 10000 blinks 2~3 times per second.
Led parkingLed(PARKING_LED_PIN, 10000);
Led gearLed(GEAR_LED_PIN, 5000);

void setup() {
  Serial.begin(BAUD_RATE);
  // put your setup code here, to run once:
  delay(2000);
  SetupServos();
  SetupLed();
  readBuf[0] = 0;
}

void loop() {
#ifdef CALIBRATE
    char val = 90;
    if (Serial.available() > 0) {
      // Read the angle.
      Serial.readBytes(&val, 1);
      if (flapServo.attached()) {
        flapServo.write(val);
      }
      Serial.println(val);
    }
    parkingLed.Update(LED_BLINK);
    gearLed.Update(LED_BLINK);
#else
  // put your main code here, to run repeatedly:
   CheckSerial();
   UpdateInstruments();
#endif
}

void SetupServos() {
  if (TRIM_SERVO_PIN >= 0) {
    trimServo.attach(TRIM_SERVO_PIN);
    delay(1000);
    if (trimServo.attached()) {
      Serial.println("Attached trim servo.");
    } else {
      Serial.println("Failed to attach trim servo.");
    }
  }
  if (FLAP_SERVO_PIN >= 0) {
    flapServo.attach(FLAP_SERVO_PIN);
    delay(1000);
    if (flapServo.attached()) {
      Serial.println("Attached flap servo.");
    } else {
      Serial.println("Failed to attach flap servo.");
    }
  }
}

void SetupLed() {
  parkingLed.Setup();
  gearLed.Setup();
  parkingLed.Update(LED_ON);
  gearLed.Update(LED_ON);
}

void CheckSerial() {
  if (Serial.available() > 0) {
    bytesRead = Serial.readBytes((char*)&simData, bufSize);
  }
}

void UpdateInstruments() {
  if (bytesRead != bufSize) return;
  // New data received in simData. Needs to update panel.
  int trimServoAngle = convertTrimAngle(simData.trimPos);
  if (trimServoAngle != panel.trimServoAngle) {
    panel.trimServoAngle = trimServoAngle;
    UpdateServo(trimServo, trimServoAngle, TRIM_MAX_ANGLE, TRIM_MIN_ANGLE);
        Serial.println("Updated trim servo.");

  }
  int flapServoAngle = convertFlapAngle(simData.flapCnt, simData.flapPos);
  if (flapServoAngle != panel.flapServoAngle) {
    panel.flapServoAngle = flapServoAngle;
    UpdateServo(flapServo, flapServoAngle, FLAP_MAX_ANGLE, FLAP_MIN_ANGLE);
    Serial.println("Updated flap servo.");
  }
  // LEDs
  UpdateLed();
}

// trimPos is -100 to 100. -100 means nose down.
int convertTrimAngle(char trimPos) {
  // Need to revert the ratio so trim "up" will point down.
  double ratio = 1.0 - ((double)trimPos + 100.0)/200.0;
  return (int)round(ratio*(TRIM_MAX_ANGLE-TRIM_MIN_ANGLE) + TRIM_MIN_ANGLE);
}

int convertFlapAngle(char flapCnt, char flapPos) {
  double ratio = (double)flapPos/(double)flapCnt;
  return (int)round(ratio*(FLAP_MAX_ANGLE - FLAP_MIN_ANGLE)) + FLAP_MIN_ANGLE;
}

void UpdateServo(const Servo& servo, int angle, int maxAngle, int minAngle) {
  if (servo.attached() && angle <= maxAngle && angle >= minAngle) {
    servo.write(angle);
  } else {
    Serial.println("Update servo failed.");
  }
}

void UpdateLed() {
  LedState parkingLightState = simData.parkingBrakeOn ? LED_ON: LED_OFF;
  parkingLed.Update(parkingLightState);

  // Gear light.
  LedState gearLightState;
  if (simData.landingGearPos == 0) {
    gearLightState = LED_OFF;
  } else if (simData.landingGearPos == 100) {
    gearLightState = LED_ON;
  } else {
    gearLightState = LED_BLINK;
  }
  gearLed.Update(gearLightState);
}

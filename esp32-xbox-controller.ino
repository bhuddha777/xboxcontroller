#include <Bluepad32.h>
#include <ESP32Servo.h>

// ---------------------------------------------------------------------------
// Pins (all safe GPIOs on the 30-pin ESP32 DevKit)
// ---------------------------------------------------------------------------
#define LED_PIN             2    // Onboard blue LED (connection indicator)
#define PIN_SUSP_LEFT       13   // SG90  - left suspension  (left stick)
#define PIN_SUSP_RIGHT      14   // SG90  - right suspension (right stick)
#define PIN_STEERING        27   // MG996R - steering        (D-pad)

// H-bridge (L298N or similar): ENA = speed (PWM), IN1/IN2 = direction
#define PIN_MOTOR_ENA       25
#define PIN_MOTOR_IN1       26
#define PIN_MOTOR_IN2       33

// ---------------------------------------------------------------------------
// Tuning - adjust these to your mechanics
// ---------------------------------------------------------------------------
// Steering (MG996R)
const int STEER_CENTER      = 90;   // servo angle when driving straight
const int STEER_RANGE       = 35;   // +/- degrees from center (increase carefully!)
const int STEER_STEP        = 4;    // degrees per update toward the target (speed)
const bool STEER_INVERT     = false;

// Suspension (SG90)
const int SUSP_CENTER       = 90;   // angle with stick at rest
const int SUSP_RANGE        = 45;   // +/- degrees at full stick deflection
const int SUSP_STEP         = 6;    // degrees per update toward the target (speed)
const bool SUSP_LEFT_INVERT = false;
const bool SUSP_RIGHT_INVERT = true; // mirrored servo: flip if it moves the wrong way

const int STICK_DEADZONE    = 60;   // sticks: -511..512, ignore small drift

// Drive motor (triggers: RT = forward, LT = reverse)
const int  TRIGGER_DEADZONE   = 40;    // triggers: 0..1023
const int  MOTOR_MIN_PWM      = 90;    // minimum duty (0-255) that makes the motor start moving
const int  MOTOR_MAX_PWM      = 255;   // speed limit (lower it, e.g. 180, for a gentler first test)
const int  MOTOR_ACCEL_STEP   = 12;    // duty change per update (smooth acceleration)
const int  MOTOR_REVERSE_PAUSE_MS = 80; // pause when changing direction (protects the H-bridge)
const bool MOTOR_INVERT       = false; // flip if the car goes backwards when pressing RT
const int  MOTOR_PWM_FREQ     = 1000;  // Hz

// Pulse widths in microseconds
const int SG90_MIN_US       = 500;
const int SG90_MAX_US       = 2400;
const int MG996R_MIN_US     = 500;
const int MG996R_MAX_US     = 2500;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
ControllerPtr controllers[BP32_MAX_GAMEPADS];

Servo servoSuspLeft;
Servo servoSuspRight;
Servo servoSteering;

int angleSuspLeft  = SUSP_CENTER;
int angleSuspRight = SUSP_CENTER;
int angleSteering  = STEER_CENTER;

ESP32PWM motorPwm;
int motorSpeed = 0;          // current signed speed: -255..255

// Move 'current' toward 'target' by at most 'step' degrees
int approach(int current, int target, int step) {
  if (current < target) return min(current + step, target);
  if (current > target) return max(current - step, target);
  return current;
}

// Stick value (-511..512) -> servo target angle, with dead zone
int stickToAngle(int value, int center, int range, bool invert) {
  if (abs(value) < STICK_DEADZONE) return center;
  // Push stick up (negative Y) -> positive movement
  int offset = map(-value, -512, 512, -range, range);
  if (invert) offset = -offset;
  return constrain(center + offset, center - range, center + range);
}

// Apply a signed speed (-255..255) to the H-bridge
void setMotor(int speed) {
  speed = constrain(speed, -255, 255);
  if (MOTOR_INVERT) speed = -speed;
  if (speed > 0) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  } else if (speed < 0) {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, HIGH);
  } else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  }
  motorPwm.write(abs(speed));
}

// Stop the motor immediately (coast)
void stopMotor() {
  motorSpeed = 0;
  setMotor(0);
}

// Active brake: both inputs high shorts the motor terminals
void brakeMotor() {
  motorSpeed = 0;
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  motorPwm.write(255);
}

void centerAllServos() {
  angleSuspLeft  = SUSP_CENTER;
  angleSuspRight = SUSP_CENTER;
  angleSteering  = STEER_CENTER;
  servoSuspLeft.write(angleSuspLeft);
  servoSuspRight.write(angleSuspRight);
  servoSteering.write(angleSteering);
}

// ---------------------------------------------------------------------------
// Bluepad32 callbacks
// ---------------------------------------------------------------------------
void onConnected(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == nullptr) {
      Serial.printf("Controller connected in slot %d: %s\n", i, ctl->getModelName().c_str());
      controllers[i] = ctl;
      ctl->playDualRumble(0, 300, 0x80, 0x80);  // rumble to confirm
      digitalWrite(LED_PIN, HIGH);
      return;
    }
  }
  Serial.println("No free slot for the controller.");
}

void onDisconnected(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == ctl) {
      Serial.printf("Controller disconnected from slot %d\n", i);
      controllers[i] = nullptr;
      digitalWrite(LED_PIN, LOW);
      centerAllServos();  // fail-safe: return everything to center
      stopMotor();        // fail-safe: stop the car
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Controller logic
// ---------------------------------------------------------------------------
void processGamepad(ControllerPtr ctl) {
  // --- Steering: D-pad left/right ---
  uint8_t d = ctl->dpad();
  int steerTarget = STEER_CENTER;
  if (d & DPAD_LEFT)  steerTarget = STEER_CENTER - STEER_RANGE;
  if (d & DPAD_RIGHT) steerTarget = STEER_CENTER + STEER_RANGE;
  if (STEER_INVERT) steerTarget = 2 * STEER_CENTER - steerTarget;
  angleSteering = approach(angleSteering, steerTarget, STEER_STEP);
  servoSteering.write(angleSteering);

  // --- Suspension: left stick Y -> left servo, right stick Y -> right servo ---
  int leftTarget  = stickToAngle(ctl->axisY(),  SUSP_CENTER, SUSP_RANGE, SUSP_LEFT_INVERT);
  int rightTarget = stickToAngle(ctl->axisRY(), SUSP_CENTER, SUSP_RANGE, SUSP_RIGHT_INVERT);
  angleSuspLeft  = approach(angleSuspLeft,  leftTarget,  SUSP_STEP);
  angleSuspRight = approach(angleSuspRight, rightTarget, SUSP_STEP);
  servoSuspLeft.write(angleSuspLeft);
  servoSuspRight.write(angleSuspRight);

  // --- Traction: RT = forward, LT = reverse (analog) ---
  int rt = ctl->throttle();   // 0..1023
  int lt = ctl->brake();      // 0..1023
  int target = 0;
  if (rt > TRIGGER_DEADZONE && rt >= lt) {
    target = map(rt, TRIGGER_DEADZONE, 1023, MOTOR_MIN_PWM, MOTOR_MAX_PWM);
  } else if (lt > TRIGGER_DEADZONE) {
    target = -map(lt, TRIGGER_DEADZONE, 1023, MOTOR_MIN_PWM, MOTOR_MAX_PWM);
  }

  if (ctl->b()) {
    // Button B: active brake
    brakeMotor();
  } else {
    // Direction change: stop briefly first to protect the H-bridge
    if ((motorSpeed > 0 && target < 0) || (motorSpeed < 0 && target > 0)) {
      stopMotor();
      delay(MOTOR_REVERSE_PAUSE_MS);
    }
    // Accelerate smoothly, but release the throttle stops quickly
    if (target == 0) motorSpeed = approach(motorSpeed, 0, MOTOR_ACCEL_STEP * 3);
    else             motorSpeed = approach(motorSpeed, target, MOTOR_ACCEL_STEP);
    // Don't hover below the minimum duty while ramping from/to zero
    if (abs(motorSpeed) > 0 && abs(motorSpeed) < MOTOR_MIN_PWM) {
      motorSpeed = (target == 0) ? 0 : (motorSpeed > 0 ? MOTOR_MIN_PWM : -MOTOR_MIN_PWM);
    }
    setMotor(motorSpeed);
  }

  // --- Extras ---
  // Button A: re-center the servos
  if (ctl->a()) {
    angleSuspLeft  = SUSP_CENTER;
    angleSuspRight = SUSP_CENTER;
    angleSteering  = STEER_CENTER;
  }

  // Debug output
  Serial.printf("Steer:%3d | SuspL:%3d SuspR:%3d | Motor:%4d | LY:%4d RY:%4d\n",
                angleSteering, angleSuspLeft, angleSuspRight, motorSpeed,
                ctl->axisY(), ctl->axisRY());
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // ESP32Servo: reserve PWM timers and use standard 50 Hz servo signals
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoSuspLeft.setPeriodHertz(50);
  servoSuspRight.setPeriodHertz(50);
  servoSteering.setPeriodHertz(50);
  servoSuspLeft.attach(PIN_SUSP_LEFT,   SG90_MIN_US,   SG90_MAX_US);
  servoSuspRight.attach(PIN_SUSP_RIGHT, SG90_MIN_US,   SG90_MAX_US);
  servoSteering.attach(PIN_STEERING,    MG996R_MIN_US, MG996R_MAX_US);
  centerAllServos();

  // H-bridge
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  motorPwm.attachPin(PIN_MOTOR_ENA, MOTOR_PWM_FREQ, 8);  // 8-bit duty: 0..255
  stopMotor();

  Serial.printf("Bluepad32 firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* mac = BP32.localBdAddress();
  Serial.printf("ESP32 Bluetooth MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  BP32.setup(&onConnected, &onDisconnected);

  // Uncomment to erase saved pairings (use if reconnecting fails)
  // BP32.forgetBluetoothKeys();

  BP32.enableVirtualDevice(false);
}

void loop() {
  if (BP32.update()) {
    for (auto ctl : controllers) {
      if (ctl && ctl->isConnected() && ctl->hasData() && ctl->isGamepad()) {
        processGamepad(ctl);
      }
    }
  }
  delay(20);
}

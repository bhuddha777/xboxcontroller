#include <Bluepad32.h>

#define LED_PIN 2   // Onboard blue LED on the DevKit

ControllerPtr controllers[BP32_MAX_GAMEPADS];

// Called when a controller connects
void onConnected(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == nullptr) {
      Serial.printf("Controller connected in slot %d: %s\n", i, ctl->getModelName().c_str());
      controllers[i] = ctl;
      ctl->playDualRumble(0, 300, 0x80, 0x80);  // rumble to confirm
      return;
    }
  }
  Serial.println("No free slot for the controller.");
}

// Called when a controller disconnects
void onDisconnected(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == ctl) {
      Serial.printf("Controller disconnected from slot %d\n", i);
      controllers[i] = nullptr;
      return;
    }
  }
}

void processGamepad(ControllerPtr ctl) {
  // Sticks: -511 to 512 | Triggers: 0 to 1023
  int lx = ctl->axisX();
  int ly = ctl->axisY();
  int rx = ctl->axisRX();
  int ry = ctl->axisRY();
  int lt = ctl->brake();     // left trigger (LT)
  int rt = ctl->throttle();  // right trigger (RT)

  // Button A turns on the onboard LED
  digitalWrite(LED_PIN, ctl->a() ? HIGH : LOW);

  // Button B rumbles the controller
  if (ctl->b()) ctl->playDualRumble(0, 150, 0xC0, 0xC0);

  // D-pad
  uint8_t d = ctl->dpad();
  if (d & DPAD_UP)    Serial.println("Up");
  if (d & DPAD_DOWN)  Serial.println("Down");
  if (d & DPAD_LEFT)  Serial.println("Left");
  if (d & DPAD_RIGHT) Serial.println("Right");

  Serial.printf(
    "A:%d B:%d X:%d Y:%d LB:%d RB:%d L3:%d R3:%d | "
    "LX:%4d LY:%4d RX:%4d RY:%4d | LT:%4d RT:%4d | "
    "View:%d Menu:%d Xbox:%d\n",
    ctl->a(), ctl->b(), ctl->x(), ctl->y(),
    ctl->l1(), ctl->r1(), ctl->thumbL(), ctl->thumbR(),
    lx, ly, rx, ry, lt, rt,
    ctl->miscSelect(), ctl->miscStart(), ctl->miscSystem());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.printf("Bluepad32 firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* mac = BP32.localBdAddress();
  Serial.printf("ESP32 Bluetooth MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  BP32.setup(&onConnected, &onDisconnected);

  // Uncomment to erase saved pairings (use if reconnecting fails)
  // BP32.forgetBluetoothKeys();

  BP32.enableVirtualDevice(false);  // don't treat touchpad/mouse as an extra device
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

#include <Bluepad32.h>

#define LED_PIN 2   // LED azul da DevKit

ControllerPtr controles[BP32_MAX_GAMEPADS];

void aoConectar(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controles[i] == nullptr) {
      Serial.printf("Controle conectado no slot %d: %s\n", i, ctl->getModelName().c_str());
      controles[i] = ctl;
      ctl->playDualRumble(0, 300, 0x80, 0x80);
      return;
    }
  }
  Serial.println("Sem slot livre para o controle.");
}

void aoDesconectar(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controles[i] == ctl) {
      Serial.printf("Controle desconectado do slot %d\n", i);
      controles[i] = nullptr;
      return;
    }
  }
}

void processarGamepad(ControllerPtr ctl) {
  // Analógicos: -511 a 512 | Gatilhos: 0 a 1023
  int lx = ctl->axisX();
  int ly = ctl->axisY();
  int rx = ctl->axisRX();
  int ry = ctl->axisRY();
  int lt = ctl->brake();
  int rt = ctl->throttle();

  // Botão A acende o LED da placa
  digitalWrite(LED_PIN, ctl->a() ? HIGH : LOW);

  // Botão B vibra o controle
  if (ctl->b()) ctl->playDualRumble(0, 150, 0xC0, 0xC0);

  uint8_t d = ctl->dpad();
  if (d & DPAD_UP)    Serial.println("Cima");
  if (d & DPAD_DOWN)  Serial.println("Baixo");
  if (d & DPAD_LEFT)  Serial.println("Esquerda");
  if (d & DPAD_RIGHT) Serial.println("Direita");

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

  Serial.printf("Firmware Bluepad32: %s\n", BP32.firmwareVersion());
  const uint8_t* mac = BP32.localBdAddress();
  Serial.printf("MAC Bluetooth da ESP32: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  BP32.setup(&aoConectar, &aoDesconectar);
  // BP32.forgetBluetoothKeys();  // descomente para apagar pareamentos salvos
  BP32.enableVirtualDevice(false);
}

void loop() {
  if (BP32.update()) {
    for (auto ctl : controles) {
      if (ctl && ctl->isConnected() && ctl->hasData() && ctl->isGamepad()) {
        processarGamepad(ctl);
      }
    }
  }
  delay(20);
}

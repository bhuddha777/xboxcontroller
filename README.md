# Controle Xbox (modelo 1914) + ESP32 DevKit 30 pinos

Status: **funcionando** (testado: botão A acende o LED no GPIO 2).

## Setup
- Controle Xbox Series X|S (modelo 1914) com firmware atualizado (Bluetooth LE) via app "Acessórios do Xbox".
- Arduino IDE: URL adicional de placas
  `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json`
- Instalar "esp32_bluepad32" e selecionar a placa **ESP32 Dev Module**.
- Pareamento: ligar o controle (botão Xbox) e segurar o botão de pareamento (ao lado da USB-C) até piscar rápido.
- Se não reconectar: descomentar `BP32.forgetBluetoothKeys();` e reenviar.

## Próximos passos (ideias)
- Carrinho com ponte H (L298N/TB6612): analógico esquerdo -> velocidade/direção (PWM, com zona morta).
- Servos com `ESP32Servo` mapeando `axisRX()` para 0-180.
- Gatilhos (`brake()`/`throttle()`, 0-1023) para aceleração gradual.
- Evitar GPIOs 6-11 (flash) e cuidado com 0, 2 e 12 (boot).

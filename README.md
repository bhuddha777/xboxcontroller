# Xbox Controller + ESP32 DevKit (30 pin)

Connects an Xbox Wireless Controller (model 1914, Xbox Series X|S) directly to an
ESP32 DevKit over Bluetooth LE using [Bluepad32](https://github.com/ricardoquesada/bluepad32).

Status: **working** (tested: pressing A turns on the onboard LED on GPIO 2).

## Setup

1. **Update the controller firmware** (needed for Bluetooth LE): install the
   "Xbox Accessories" app from the Microsoft Store, connect the controller via USB
   and update it.
2. **Arduino IDE:** add this URL under *File > Preferences > Additional boards manager URLs*:
   `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json`
3. Install **esp32_bluepad32** in the Boards Manager and select the board **ESP32 Dev Module**.
4. Upload `esp32-xbox-controller.ino` and open the Serial Monitor at 115200 baud.

## Pairing

Turn on the controller (Xbox button), then hold the pairing button (next to the USB-C port)
until the Xbox logo blinks fast. The controller rumbles when connected and the Serial Monitor
prints "Controller connected". It reconnects automatically afterwards.

The controller stores a single Bluetooth pairing: pairing it with the ESP32 removes the
pairing with the PC (USB still works).

## Troubleshooting

- Make sure the controller firmware is up to date.
- Uncomment `BP32.forgetBluetoothKeys();` in `setup()`, upload, and pair again.
- Use a USB data cable and a good power source; Bluetooth draws a lot of current.

## Next steps (ideas)

- RC car with an H-bridge (L298N / TB6612): left stick -> speed/steering (PWM, with a dead zone).
- Servos with `ESP32Servo`, mapping `axisRX()` to 0-180 degrees.
- Triggers (`brake()` / `throttle()`, 0-1023) for gradual acceleration.
- Avoid GPIOs 6-11 (flash) and be careful with 0, 2 and 12 (boot strapping pins).

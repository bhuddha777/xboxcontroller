# Xbox Controller + ESP32 DevKit (30 pin)

Connects an Xbox Wireless Controller (model 1914, Xbox Series X|S) directly to an
ESP32 DevKit over Bluetooth LE using [Bluepad32](https://github.com/ricardoquesada/bluepad32).

Status: **base connection tested and working** (A button lit the onboard LED).
The servo code (steering + suspension) is **new and untested on hardware**.

## Controls

| Input | Action |
|-------|--------|
| D-pad left / right | Steering (MG996R), smooth movement, returns to center on release |
| Left stick up / down | Left suspension (SG90) |
| Right stick up / down | Right suspension (SG90) |
| A button | Re-center all servos |
| Controller disconnects | All servos return to center (fail-safe) |

## Wiring

| Servo | Signal pin (ESP32) |
|-------|--------------------|
| SG90 left suspension | GPIO 13 |
| SG90 right suspension | GPIO 14 |
| MG996R steering | GPIO 27 |

**Power - important:**
- Do **not** power the servos from the ESP32 5V/VIN pin. The MG996R can draw ~2.5 A when stalled.
- Use a separate 5-6 V supply (BEC or battery + regulator) rated for **at least 3 A**.
- Connect the **GND of the servo supply to the ESP32 GND** (common ground), or the signal will not work.
- Add a 470-1000 uF capacitor across the servo supply near the servos to avoid brown-outs/resets.
- The ESP32 is powered separately (USB or its own regulator).

## Tuning

All adjustable values are at the top of `esp32-xbox-controller.ino`: center angles, ranges, speeds,
dead zone and `*_INVERT` flags. Start with small ranges to avoid forcing the mechanics, then increase.


## Setup

1. **Update the controller firmware** (needed for Bluetooth LE): install the
   "Xbox Accessories" app from the Microsoft Store, connect the controller via USB
   and update it.
2. **Arduino IDE:** add this URL under *File > Preferences > Additional boards manager URLs*:
   `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json`
3. Install **esp32_bluepad32** in the Boards Manager and select the board **ESP32 Dev Module**.
4. Install the **ESP32Servo** library (Library Manager).
5. Upload `esp32-xbox-controller.ino` and open the Serial Monitor at 115200 baud.

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

- Drive motors with an H-bridge (L298N / TB6612), speed on the triggers (`throttle()` / `brake()`).
- Avoid GPIOs 6-11 (flash) and be careful with 0, 2 and 12 (boot strapping pins).

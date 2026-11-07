# Xbox Controller + ESP32 DevKit (30 pin)

Connects an Xbox Wireless Controller (model 1914, Xbox Series X|S) directly to an
ESP32 DevKit over Bluetooth LE using [Bluepad32](https://github.com/ricardoquesada/bluepad32).

Status: **base connection tested and working** (A button lit the onboard LED).
Steering and suspension servos were tested (SG90 are mirrored, so the right one is inverted in code).
The drive motor code (H-bridge) is **new and untested on hardware**.

## Controls

| Input | Action |
|-------|--------|
| D-pad left / right | Steering (MG996R), smooth movement, returns to center on release |
| Left stick up / down | Left suspension (SG90) |
| Right stick up / down | Right suspension (SG90) |
| Right trigger (RT) | Drive forward (analog: the harder you press, the faster) |
| Left trigger (LT) | Drive in reverse (analog) |
| B button | Active brake |
| A button | Re-center the servos |
| Controller disconnects | Servos return to center and the motor stops (fail-safe) |

## Wiring

| Servo | Signal pin (ESP32) |
|-------|--------------------|
| SG90 left suspension | GPIO 13 |
| SG90 right suspension | GPIO 14 |
| MG996R steering | GPIO 27 |
| H-bridge ENA (speed, PWM) | GPIO 25 |
| H-bridge IN1 | GPIO 26 |
| H-bridge IN2 | GPIO 33 |

H-bridge (L298N): motor on OUT1/OUT2, ENA/IN1/IN2 to the ESP32 pins above (remove the ENA jumper
so the PWM pin controls speed). Motor battery on the 12V/+ terminal, and **its GND to the ESP32 GND**.
If the car goes backwards when pressing RT, swap the motor wires or set `MOTOR_INVERT = true`.

**Power - important:**
- Do **not** power the servos from the ESP32 5V/VIN pin. The MG996R can draw ~2.5 A when stalled.
- Use a separate 5-6 V supply (BEC or battery + regulator) rated for **at least 3 A**.
- Connect the **GND of the servo supply to the ESP32 GND** (common ground), or the signal will not work.
- Add a 470-1000 uF capacitor across the servo supply near the servos to avoid brown-outs/resets.
- The ESP32 is powered separately (USB or its own regulator).
- Motors and servos make electrical noise. Ideally power the **drive motor from its own battery**,
  separate from the servo supply, with all grounds joined together.
- No capacitor yet: the MG996R can pull the ESP32 voltage down and drop the Bluetooth connection.
  Powering the servos from a separate supply (not the ESP32 5V pin) is the fix; the capacitor is a
  recommended addition. A 100 nF ceramic capacitor across the motor terminals also reduces noise.

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

- Add a second drive motor (differential/4x4) or a lights/horn output on spare buttons.
- Add the supply capacitor and a battery voltage monitor (ADC) with a low-battery warning.
- Avoid GPIOs 6-11 (flash) and be careful with 0, 2 and 12 (boot strapping pins).

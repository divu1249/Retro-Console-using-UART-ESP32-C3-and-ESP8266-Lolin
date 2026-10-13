# Hardware Assembly and Wiring

## Overview

The console uses an ESP32-C3 and an ESP8266 as separate
microcontroller subsystems, split by job:

| Board | Role |
|---|---|
| **ESP32-C3 SuperMini** | Display, rendering, UI and game logic |
| **ESP8266 (LoLin / NodeMCU v3, CH340)** | Button scanning, buzzer, battery-voltage sensing |

The two boards share one regulated 3.3 V rail and one common ground, and talk
over a single hardware UART link. Neither board's onboard regulator is used
in normal operation: the console's own regulator feeds both boards' `3V3`
pins directly.

```
 18650 ─► TP4056 ─► slide switch ─► AMS1117-3.3 ─┬─► ESP32-C3  3V3
                                                 ├─► ESP8266   3V3
                                                 └─► TFT       VCC + LED

 ESP32-C3 GPIO20 (RX) ◄───────── TX (GPIO1)  ESP8266
 ESP32-C3 GPIO21 (TX) ─────────► RX (GPIO3)  ESP8266
```

Why the split: the SuperMini exposes only 13 GPIOs (GPIO0–10, 20, 21;
GPIO18/19 are not broken out), three of which (2, 8, 9) are strapping pins.
The TFT and UART consume 7 of those pins, so the six buttons, the buzzer and
battery sensing live on the ESP8266 instead.

This document explains how the parts connect. For the compact pin table, see
the pinout reference.

## Display

**Module:** 1.8" TFT, ST7735R controller, 128×160, 4-wire SPI breakout
(8 pins: `LED, SCK, SDA, A0/DC, RESET, CS, GND, VCC`).

**Interface:** SPI write-only. There is no MISO line. The firmware uses the
Adafruit_ST7735 library in **software SPI** mode, with `INITR_BLACKTAB` and
`setRotation(0)` (portrait, header pins at the bottom).

| TFT pin | ESP32-C3 GPIO | Notes |
|---|---|---|
| VCC | 3V3 | Regulated rail from the AMS1117 |
| GND | GND | Common ground |
| CS | 7 | |
| DC (A0) | 6 | |
| RESET | 10 | Driven by firmware; a floating RESET leaves the panel uninitialised |
| SDA (MOSI) | 4 | |
| SCK | 5 | |
| LED (backlight) | 3V3 | Tied directly to the rail, no PWM dimming |

**Why software SPI:** the ESP32-C3 Arduino core's default hardware SPI pins
are SCK=4, MISO=5, MOSI=6, SS=7. This wiring puts MOSI on GPIO4, SCK on
GPIO5 and DC on GPIO6, so the default bus would collide with DC. Passing the
pins explicitly to the constructor,
`Adafruit_ST7735(CS, DC, MOSI, SCLK, RST)`, avoids the clash.

**Symptoms that point at the display wiring:**

- Backlight on, panel stays white: init commands are not arriving. Check
  RESET, CS, DC, SDA and SCK continuity, and that the TFT ground is joined
  to the ESP32-C3 ground. Also see the brownout note under Power.

## Controls

Six logical buttons (UP, DOWN, LEFT, RIGHT, A, B), all wired to the
**ESP8266**, not the ESP32-C3. Button state is debounced (25 ms) on the
ESP8266 and sent to the ESP32-C3 over UART.

| Button | NodeMCU pin | GPIO | Switch leg 1 | Switch leg 2 | Pull configuration |
|---|---|---|---|---|---|
| UP | D2 | 4 | GPIO | GND | Internal pull-up |
| DOWN | D7 | 13 | GPIO | GND | Internal pull-up |
| LEFT | D3 | 0 | GPIO | GND | Internal pull-up (strapping pin) |
| RIGHT | D4 | 2 | GPIO | GND | Internal pull-up (strapping pin) |
| A | D8 | 15 | GPIO | **3V3** | Board's onboard pull-down; **active-high** (strapping pin) |
| B | D0 | 16 | GPIO | GND | **External 10 kΩ pull-up** to 3V3 |

### Details that differ from the "normal" button

**Button A is wired to 3V3, not GND.** GPIO15 must read LOW at boot on the
ESP8266, so it idles low through the board's onboard pull-down (typically
about 10 kΩ) and the button pulls it high. In firmware this is the only
button with `activeHigh = true`.

Note: the current ESP8266 firmware calls `pinMode(..., INPUT_PULLUP)` on
every button pin except GPIO16, which includes GPIO15. That works on this
build, but it makes the internal pull-up fight the board's pull-down and
leaves GPIO15's idle level marginal. Plain `INPUT` on GPIO15 matches the
wiring more cleanly.

**Button B needs an external resistor.** GPIO16 has no internal pull-up.
Place a 10 kΩ resistor from the GPIO16 junction (the same point as the
button's signal leg) to 3V3. Configure it as `INPUT`, not `INPUT_PULLUP`.

```
        3V3 ──[10 kΩ]──┬── D0 (GPIO16)
                       │
                    [Button B]
                       │
                      GND
```

### 4-leg tactile switches

A 4-leg tactile switch has two internally joined pairs of legs. Pressing the
button bridges the pairs. Use **one leg from each pair**; using two legs
from the same pair gives no extra connection. Before soldering, check with a
multimeter in continuity mode: the two pairs must show **no continuity while
the button is unpressed**. A switch that beeps unpressed is defective (one
unit in this build was, and it held GPIO16 permanently low).

### Boot-time constraints (ESP8266 strapping pins)

| GPIO | Required level at reset | Button on it |
|---|---|---|
| 0 | HIGH | LEFT |
| 2 | HIGH | RIGHT |
| 15 | LOW | A |

Do not hold LEFT, RIGHT or A while powering on or resetting. Released
buttons do not interfere. GPIO9 and GPIO10 are wired to the ESP8266's flash
and must not be used.

**If the ESP8266 shows no serial output and the buzzer never plays,** a
button wire is holding a strapping pin at the wrong level. Disconnect all
button wires, confirm it boots, then reconnect them one at a time. In this
build the culprit was Button A's wires being on the wrong rails, which held
GPIO15 high at boot.

Pressing RIGHT also flashes the NodeMCU's blue onboard LED, because that LED
shares GPIO2. This is expected.

## Inter-Microcontroller Communication

A single hardware UART link, 115200 baud, 8N1, 3.3 V logic on both sides.
TX goes to RX and RX goes to TX.

| ESP8266 (NodeMCU) | ESP32-C3 SuperMini |
|---|---|
| **TX** (GPIO1, the pad silkscreened `TX`) | GPIO20 (RX) |
| **RX** (GPIO3, the pad silkscreened `RX`) | GPIO21 (TX) |
| GND | GND |

**Use the labelled TX/RX pads, not D1.** On the ESP8266 the `Serial` object
is hard-wired to GPIO1 (TX) and GPIO3 (RX). GPIO5 (D1) has no UART function,
so a wire on D1 never carries serial data. The link only started working
once the wires moved to the labelled TX/RX pads.

On the ESP32-C3 the link uses a second serial port,
`HardwareSerial LinkSerial(1)` on GPIO20/21, so the USB `Serial` stays free
for debugging.

**Message format** (newline-terminated ASCII):

| Direction | Message | Meaning |
|---|---|---|
| ESP8266 → ESP32-C3 | `B:<NAME>:<0\|1>` | Button state change, for example `B:UP:1` |
| ESP8266 → ESP32-C3 | `V:<percent>` | Battery percentage, every 5 s |
| ESP32-C3 → ESP8266 | `S:<freq>:<ms>` | Play a buzzer tone |
| ESP32-C3 → ESP8266 | `M:<ms>` | Motor pulse (currently ignored, see Assembly Notes) |

`println()` ends lines with `\r\n`; the parsers trim the `\r`.

**USB conflict:** the ESP8266's USB-serial chip uses the same GPIO1/GPIO3
lines. While the link wires are connected, do not run the ESP8266 from USB
and expect the link to work. Run it from the battery rail, or disconnect the
RX wire when flashing or debugging over USB.

## Audio

**Buzzer:** passive piezo buzzer.

| Buzzer pin | Connects to |
|---|---|
| + | ESP8266 D5 (GPIO14) |
| − | GND |

It is driven directly from the GPIO with the Arduino `tone()` function. No
driver transistor is used because a small passive buzzer draws well under
the pin's current limit. It must be a **passive** buzzer: an active buzzer
would ignore the frequency and emit one fixed pitch.

In the console firmware the buzzer plays the tones the ESP32-C3 requests
with `S:<freq>:<ms>` (menu and game sound effects).

## Power

**Source:** one 18650 Li-ion cell, 3.7 V nominal (3.0 V empty, 4.2 V full),
2200 mAh rated.

### Power chain

```
                    ┌──────────────┐
  18650 + ────────► │ B+      OUT+ ├──► slide switch ──► AMS1117 VIN
  18650 − ────────► │ B−      OUT− ├──► common GND
                    │   TP4056     │
  USB ────────────► │ IN (charge)  │
                    └──────────────┘

  AMS1117 VOUT ─┬──► ESP32-C3 3V3
                ├──► ESP8266 3V3
                ├──► TFT VCC and LED
                ├──► Button A common leg, Button B pull-up resistor
                ├── 470 µF electrolytic ── GND   (bulk)
                └── 100 nF ceramic ─────── GND   (decoupling, optional)
```

- **TP4056** charges the cell from USB. `OUT+` and `OUT−` pass the battery
  voltage through, unregulated.
- **The slide switch is on the positive line only, after the TP4056.**
  Ground is never switched. Because the switch sits after the charger, the
  cell charges while the console is off.
- **AMS1117-3.3** regulates the rail. Both boards' `3V3` pins are fed
  directly from it, bypassing the boards' own regulators. Never feed battery
  voltage into either board's `5V`/`VIN` pin.

<!-- TODO: record whether the TP4056 module is the protected variant (extra DW01/FS8205 chips) -->

### Capacitors (required)

Place the capacitors directly across the AMS1117 **VOUT** and **GND**, as
close to the regulator as the board layout allows.

| Part | Value | Polarity |
|---|---|---|
| Bulk | 470 µF electrolytic | `+` leg (longer, no stripe) to VOUT; stripe side to GND |
| Decoupling | 100 nF ceramic | None |

Without the bulk capacitor, the console on battery power often (about 97 %
of cold boots in testing) came up with the backlight on and a plain white
screen, and only booted after a charger was briefly plugged in. The cause
was the rail sagging during power-on inrush while the display was being
initialised, so the init sequence was lost. The firmware also waits 300 ms
before `tft.initR()` to let the rail settle.

<!-- TODO: record the capacitor values actually fitted if they differ from the above -->

### Ground

One common ground ties together: TP4056 `OUT−`, AMS1117 GND, both boards'
GND, the TFT GND, every button's common leg, the buzzer and the capacitor
negatives. Use a continuous ground bus rather than chaining ground from
part to part, and **verify with continuity mode that the two boards' GND
pins are connected** before debugging anything on the UART link.

### Battery-voltage sensing

A 50 % divider from the battery rail into the ESP8266's `A0` pin:

```
  Battery + ──[100 kΩ]──┬──[100 kΩ]── GND
                        │
                       A0
```

The ESP8266 samples `A0` every 5 seconds, doubles the result to undo the
divider, and sends a percentage to the ESP32-C3 (linear 3.0 V to 0 %, 4.2 V
to 100 %).

<!-- TODO: record whether the divider is tapped before or after the slide switch -->

**Accuracy caveat:** NodeMCU boards normally have their own on-board divider
(about 220 kΩ / 100 kΩ) in front of the ADC, scaled to a 0–3.3 V input.
A 200 kΩ external divider has enough source resistance to be loaded by that
network, so the reading can come out roughly 10–15 % low. Check the reported
voltage against a multimeter and either add a calibration factor in
firmware or use lower-value divider resistors (for example 10 kΩ + 10 kΩ).

### Indicator LEDs

The LEDs are wired straight across the switched battery rail and are not
GPIO-controlled, so they are on whenever the console is on.

<!-- TODO: record the LED current-limiting resistor value(s) -->

### Voltage ratings and the regulator's limits

| Point | Normal range |
|---|---|
| Battery / TP4056 OUT | 3.0–4.2 V |
| 3.3 V rail (AMS1117 out) | at most about 3.3 V, lower as the battery drains |
| GPIO logic (both boards) | 3.3 V |

The AMS1117 needs roughly 1.1 V of headroom, and a full cell is only 4.2 V,
so the rail never reaches a clean 3.3 V from this battery. A measured
example: a 3.8 V cell gave about 3.0 V at the AMS1117 output. Expect
instability as the cell drains, and treat the bottom of the discharge
curve as unusable. A low-dropout regulator (for example HT7333 or an
ME6211-class part) or a buck-boost converter is the clean upgrade.

## Assembly Notes

**Power-up order and USB.** Switch the console **off** before plugging USB
into either board. With the switch on, the AMS1117 and the board's own USB
regulator would both drive the same 3V3 net.

**Build and test in this order.** Each stage can be verified alone, which
makes faults easy to isolate:

1. Display alone on the ESP32-C3 (solid-colour fill test).
2. ESP8266 alone: buttons and buzzer, reading pins over its own USB serial.
3. UART link: dump raw bytes on the ESP32-C3 and confirm `B:UP:1` arrives
   before running any parsing code.
4. Full firmware on both boards, with the ESP8266 on battery power.
5. Battery sensing last.

**Pins deliberately left unused.**

| Board | Pins | Reason |
|---|---|---|
| ESP32-C3 | GPIO0, 1, 2, 3, 8, 9 | Spare; 2, 8, 9 are strapping pins |
| ESP8266 | D1 (GPIO5) | Not a UART pin; do not use it for the link |
| ESP8266 | GPIO9, GPIO10 (SD2/SD3) | Connected to flash |

# Serial Communication Protocol

This document describes how the ESP32-C3 and the ESP8266 talk to each other.
It was written against the firmware on both boards: the ESP8266 button,
buzzer, motor and battery sketch, and the ESP32-C3 UI and games sketch.

## Interface

| Parameter | Value |
|---|---|
| Interface | UART, full duplex, point to point |
| Baud rate | 115200 |
| Frame | 8 data bits, no parity, 1 stop bit (8N1) |
| Logic level | 3.3 V on both boards |
| Flow control | None |
| Main console | ESP32-C3 (display, UI, games) |
| Input and peripheral controller | ESP8266 (buttons, buzzer, motor, battery sensing) |

**Connections**

| ESP8266 (NodeMCU) | ESP32-C3 SuperMini |
|---|---|
| TX (GPIO1) | GPIO20 (RX) |
| RX (GPIO3) | GPIO21 (TX) |
| GND | GND |

On the ESP8266 the link is the default `Serial` port. On the ESP32-C3 it is a
second port, `HardwareSerial LinkSerial(1)`, on GPIO20/21, so the USB `Serial`
stays free for debugging. The ESP8266's USB-serial chip shares GPIO1/GPIO3
with this link. See [`hardware/wiring.md`](../hardware/wiring.md) for the
wiring and the USB caveat.

## Message Prefixes

| Prefix | Purpose | Direction |
|---|---|---|
| `B:` | Button state change | ESP8266 → ESP32-C3 |
| `V:` | Battery percentage | ESP8266 → ESP32-C3 |
| `S:` | Play a buzzer tone | ESP32-C3 → ESP8266 |
| `M:` | Pulse the vibration motor | ESP32-C3 → ESP8266 |

## Message Direction

| Prefix | Sender | Receiver | Sent when | Receiver action |
|---|---|---|---|---|
| `B:` | ESP8266 | ESP32-C3 | On every debounced (25 ms) press or release of any of the six buttons | Updates that button's state; the game loop derives "just pressed" edges from it |
| `V:` | ESP8266 | ESP32-C3 | Every 5 s, with the first one immediately after boot | Stores the value; redraws the battery bar if the Home screen is showing, otherwise it is drawn the next time Home is shown |
| `S:` | ESP32-C3 | ESP8266 | On sound events: Snake food eaten (`S:1500:60`), game over in Snake or 2048 (`S:200:300`), each 2048 move (`S:600:40`) | Calls `tone(buzzerPin, freq, ms)`, which returns immediately (timer-driven) |
| `M:` | ESP32-C3 | ESP8266 | On haptic events: menu move (`M:20`), Snake food eaten (`M:30`), game over (`M:150`) | Drives the motor pin HIGH, waits `ms` with a blocking `delay()`, then drives it LOW |

## Message Format

**General rules**

- ASCII text, one message per line.
- Fields are separated by `:`. The first field is the prefix letter.
- Case-sensitive. No spaces inside a message.
- Senders end each message with `println()`, which emits `\r\n`.
- Receivers split on `\n` and call `trim()`, which also removes the `\r`.
- No checksum, length field or acknowledgement.

### `B:` button event (ESP8266 → ESP32-C3)

```
B:<NAME>:<STATE>
```

| Field | Valid values | Meaning |
|---|---|---|
| `NAME` | `UP`, `DOWN`, `LEFT`, `RIGHT`, `A`, `B` | Which button |
| `STATE` | `1` or `0` | `1` = pressed, `0` = released |

`STATE` is the logical state. Button A is wired active-high and Button B uses an
external pull-up, but the ESP8266 normalises both before sending. A message
is sent only when the debounced state changes. Nothing is repeated while a
button is held, and nothing is sent at boot for buttons that start released.

### `V:` battery percentage (ESP8266 → ESP32-C3)

```
V:<PERCENT>
```

| Field | Valid values | Meaning |
|---|---|---|
| `PERCENT` | Integer, 0 to 100 | Estimated charge |

The ESP8266 reads `A0`, converts it to a voltage, doubles it to undo the 50 %
divider, and maps it linearly:

```
V_pin   = analogRead(A0) / 1023 × 3.3
V_batt  = V_pin × 2.0
PERCENT = clamp( (V_batt − 3.0) / (4.2 − 3.0) × 100 , 0 , 100 )   // truncated to an integer
```

### `S:` buzzer tone (ESP32-C3 → ESP8266)

```
S:<FREQ>:<MS>
```

| Field | Valid values | Meaning |
|---|---|---|
| `FREQ` | Integer, Hz | Tone frequency. The firmware sends 200 to 1500 |
| `MS` | Integer, milliseconds | Tone duration. The firmware sends 40 to 300 |

### `M:` motor pulse (ESP32-C3 → ESP8266)

```
M:<MS>
```

| Field | Valid values | Meaning |
|---|---|---|
| `MS` | Integer, milliseconds | Motor on-time. The firmware sends 20, 30 or 150 |

## Examples

**A button press and release** (UP):

```
B:UP:1
B:UP:0
```

On the wire, `B:UP:1` is eight bytes:

```
42 3A 55 50 3A 31 0D 0A
B  :  U  P  :  1  \r \n
```

**Boot-time battery report** (the ESP8266 sends this as soon as it starts):

```
V:87
```

**Sound and haptics:**

```
S:1500:60      play 1500 Hz for 60 ms
M:30           run the motor for 30 ms
```

**A typical sequence while playing Snake:**

```
ESP8266 → ESP32-C3   B:RIGHT:1      player presses RIGHT
ESP8266 → ESP32-C3   B:RIGHT:0      player releases RIGHT
ESP32-C3 → ESP8266   S:1500:60      later, the snake eats food
ESP32-C3 → ESP8266   M:30
ESP8266 → ESP32-C3   V:84           periodic battery report, every 5 s
```

## Error Handling

How each receiver treats bad input:

- **Incomplete line:** nothing is processed until `\n` arrives. Bytes are
  appended to an Arduino `String` buffer. Neither firmware limits the line
  length, so a stream that never sends `\n` grows the buffer without bound.
- **Empty or whitespace-only line:** ignored.
- **Unknown prefix:** ignored, with no error message sent back.
- **`B:` with no second `:`** is ignored. An unknown `NAME` is ignored.
- **`B:` with a `STATE` other than `1`**, including non-numeric text, is
  treated as released.
- **`V:` is not range-checked.** Non-numeric text becomes `0` and shows as
  0 %. Values outside 0 to 100 are stored and displayed as received.
- **`S:` and `M:` are not validated.** Fields are read with `toInt()`, so
  missing or non-numeric fields become `0`.
- **Corruption:** there is no integrity check. A garbled message is dropped
  only if it no longer parses.
- **Lost messages:** there are no acknowledgements or retries. If a release
  (`B:<NAME>:0`) is lost, the ESP32-C3 keeps that button as pressed until the
  next `B:` message for the same button arrives.
- **Link loss:** the ESP32-C3 does not time out on missing `V:` messages. It
  keeps the last battery value, or `--` if none has ever arrived.

## Notes

- **Button messages are state, not an event queue.** The ESP32-C3 reads all
  pending serial data at the start of each loop iteration and computes
  "just pressed" edges once per iteration. A press and release that both
  arrive within one iteration, for example during a long screen redraw, can be
  missed.
- **The motor pulse blocks the ESP8266.** `M:<ms>` uses `delay(ms)`, so button
  scanning and battery reports pause for that long. Keep pulses short; the
  firmware uses 20 to 150 ms.
- **The ESP8266 sketch must include the `M:` branch of `handleCommand()`.** A
  build without it ignores `M:` silently, since unknown prefixes are not an
  error.
- **`V:` is an uncalibrated estimate of the cell voltage.** It does not
  measure the regulated 3.3 V rail. The AMS1117 drops out as the cell
  drains, so the console can brown out before `V:` ever reaches 0. See
  [`hardware/wiring.md`](../hardware/wiring.md).
- **Message size is small.** At 115200 baud a `B:` message takes about
  0.7 ms on the wire, so the serial link is not a meaningful source of input
  latency.
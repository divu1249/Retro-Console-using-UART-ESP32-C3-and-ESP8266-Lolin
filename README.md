# ESP Retro Console 🎮

A DIY retro gaming console built using an ESP32-C3 and an ESP8266, combining embedded C++, TFT graphics, physical controls, and serial communication between two microcontrollers.

## Overview

This project is a custom-built retro gaming console developed as a practical exploration of embedded systems, microcontroller communication, and game development.

The ESP32-C3 manages the graphical interface and game logic, while the ESP8266 handles physical button inputs, debouncing, and supported peripheral commands.

## Features

* ST7735 TFT graphical interface
* Menu-driven home screen and game selection
* Snake and 2048
* Six logical physical controls
* Dual-microcontroller architecture
* UART serial communication
* Buzzer feedback
* LittleFS-based wallpaper loading
* Built-in button-testing interface

## Demo

Project photos and gameplay demonstrations will be available in [`media/`](media/).

## Hardware

The console uses two microcontrollers, a TFT display, physical controls, a buzzer, and supporting power and wiring components.

See [`hardware/`](hardware/) for the bill of materials, schematics, pin mappings, and assembly documentation.

## Software

* C++ and the Arduino framework
* ESP32 Arduino core
* ESP8266 Arduino core
* Adafruit GFX Library
* Adafruit ST7735 and ST7789 Library
* SPI and LittleFS

## Getting Started

1. Install the Arduino IDE and the required board packages.
2. Install the display libraries listed in the setup guide.
3. Upload the ESP32-C3 firmware.
4. Upload the ESP8266 firmware.
5. Connect the boards using the documented wiring.
6. Assemble and power the console.

See [`docs/setup.md`](docs/setup.md) for detailed instructions.

## Architecture

The ESP32-C3 runs the games and graphical interface. The ESP8266 scans the controls and handles supported peripheral commands. The boards communicate over UART serial.

See [`docs/architecture.md`](docs/architecture.md) and [`docs/serial-protocol.md`](docs/serial-protocol.md).

## Known Limitations

### Startup power delivery

At lower battery voltages, the ESP32-C3 can occasionally require more initial current than the power source reliably supplies. Additional capacitors have been installed, but startup remains inconsistent under these conditions.

### Microcontroller resources

Available memory constrains the complexity and size of future games, graphics, and additional features.

## Documentation

* [System architecture](docs/architecture.md)
* [Development setup](docs/setup.md)
* [Serial communication protocol](docs/serial-protocol.md)
* [Hardware testing](docs/testing.md)
* [Pin mapping](hardware/pinout.md)
* [Wiring and assembly](hardware/wiring.md)
* [Bill of materials](hardware/bom.csv)

## Future Development

* Investigate startup power stability
* Optimize memory usage
* Expand the game library
* Improve hardware and assembly documentation
* Explore additional interface and audio features

## License

See [`LICENSE`](LICENSE) for the project's license terms.

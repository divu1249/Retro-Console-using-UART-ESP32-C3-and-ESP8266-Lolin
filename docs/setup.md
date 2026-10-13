**Development Setup**



**Requirements**



* Arduino IDE
* ESP32 Arduino board package
* ESP8266 Arduino board package
* Adafruit GFX Library
* Adafruit ST7735 and ST7789 Library
* USB data cables suitable for the development boards
* 

**Firmware Locations**



* ESP32-C3: firmware/esp32-c3-console/esp32-c3-console.ino
* ESP8266: firmware/esp8266-controller/esp8266-controller.ino
* 

**Installation**



1. Install the Arduino IDE.
2. Install the appropriate ESP32 and ESP8266 board packages.
3. Install the required Adafruit display libraries.
4. Open the ESP32-C3 sketch and select the exact board variant.
5. Compile and upload the firmware.
6. Open the ESP8266 sketch and select the correct board.
7. Compile and upload the firmware.
8. Connect the two boards according to the hardware documentation.
9. Power on the assembled console and verify normal operation.



**Wallpaper**



The ESP32-C3 firmware initializes LittleFS and attempts to load a wallpaper from /wallpaper.bin.



Document the required wallpaper format and filesystem upload procedure once confirmed. A compatible wallpaper file may be required for the intended home-screen appearance.



**Troubleshooting**



If the console does not start consistently at lower battery voltage, refer to the power-delivery notes in the hardware documentation.



For wiring and pin assignments, see ../hardware/.



**Tested Environment**

* Arduino IDE version: 2.3.10
* ESP32 board package version: 3.3.12
* ESP8266 board package version: 2.5.2
* Display library versions: 1.0.7



Replace the placeholders with the versions used to build and test the project.


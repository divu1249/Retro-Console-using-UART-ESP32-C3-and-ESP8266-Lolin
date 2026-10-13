**System Architecture**



**Overview**



The ESP Retro Console uses a dual-microcontroller architecture, separating the main gaming interface from physical input handling.



**ESP32-C3 — Main Console**



Responsibilities:



* TFT display initialization and rendering
* Home screen and menu navigation
* Game logic and state management
* Snake and 2048
* Button-test interface
* Receiving controller events over serial communication
* Sending supported peripheral commands
* ESP8266 — Input and Peripheral Controller



Responsibilities:



* Scanning physical buttons
* Debouncing inputs
* Reporting button events over serial communication
* Handling supported buzzer commands



**Communication**



The microcontrollers communicate through a serial interface configured for 115200 baud.



The ESP32-C3 uses UART1 with RX on GPIO20 and TX on GPIO21. The ESP8266 uses its Serial interface.



Refer to serial-protocol.md for the message formats and ../hardware/wiring.md for the physical connections.



**Design Rationale**



Separating input handling from the main console logic allows the system to distribute responsibilities between two microcontrollers while keeping the game interface and input subsystem distinct.



**Resource Constraints**



Available microcontroller memory limits the size and complexity of future games, graphics, and additional features.



**Power Considerations**



At lower battery voltages, the ESP32-C3 can experience inconsistent startup because the power source may not reliably meet its initial current demand. Additional capacitance has been installed, the issue is likely caused by the use of AMS1117 3.3v as the regulator which has a 1.2v drop and delivers inconsistent power when battery reachs lower voltage after discharge/use.


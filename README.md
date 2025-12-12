# Pill dispenser
This project implements a simple pill dispensing system using a Raspberry Pi Pico. It was developed as part of an Embedded Systems Programming course and demonstrates core concepts such as GPIO control, EEPROM usage, and serial communication.

<img width="500" height="253" alt="image" src="https://github.com/user-attachments/assets/684445b1-53d7-4f92-8cb0-89fafc9a450e" />
<img width="216" height="225" alt="image" src="https://github.com/user-attachments/assets/156e5abe-51b8-47d4-b9c1-7cd6e7233cb5" />

### Features
  - Pill detection via sensor input
  - LED indicators for system status (e.g. waiting for action, ready, error)
  - EEPROM integration to store persistent data (e.g. last calibration info, pill count, slot index)
  - Serial output for debugging and monitoring
  - Modular CMake-based build system for portability and clarity
### Tools Used
  - Raspberry Pi Pico
  - CMake
  - ARM GCC toolchain (arm-none-eabi-gcc)
  - CLion IDE
  - Git & GitHub
### How it works

The system is designed to dispense, detect and track pills count using a Raspberry Pi Pico. Here's how it operates:

- **Startup & Initialization**
    - On power-up, the system initializes GPIO pins, EEPROM, and serial communication.
    - LED indicators show system status: blinking while waiting for calibration, ON during work, 5 times blink in case of error.

- **Pill Detection**
    - A sensor monitors the pill compartment.
    - When a pill is detected (or missing), the system triggers the appropriate response.

- **Dispensing Logic**
    - Upon receiving a dispense command (manual or timed), the system activates a motor or actuator to release a pill.
    - LED blinks during dispensing to indicate activity.

- **EEPROM Usage**
    - Stores persistent data such as:
        - Calibration information
        - Total pills dispensed
        - Last dispense slot index in case of power reboot

- **Serial Output**
    - Sends detailed debug messages during all steps and status updates via USB serial.
    - Useful for monitoring system behavior during development.

- **Error Handling**
    - If a pill fails to dispense or sensor input is invalid, the system indicates an error and continues to next slot.
    - LED blinks 5 times to indicate the issue.




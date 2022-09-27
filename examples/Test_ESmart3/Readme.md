# ESP32 Arduino Test Firmware for the Joba_ESmart3 Library

WARNING: this firmware changes settings in the connected eSmart3 MPPT charger.
Some of the changes can only be undone by RS485 commands (i.e. by using the library or other ESmart3 setup programs).

# Installation
There are many options to compile and install an ESP32 Arduino firmware. I use this one on linux:
* Install MS Code
 * Install PlatformIO as MS Code extension
* Add this folder to the MS Code workspace
* Edit platformio.ini in this folder so the usb device name for monitor and upload matches your environment
* Select build and upload the firmware

# Connection
See library readme for wiring details. Required hardware:
* A MAX485 board (or similar) to convert RS485 signals to serial.
* A level shifter from 5V to 3.3V (at least 3 channels) if the MAX485 board does not already have one
* An ESP32 (with USB connection or an additional serial-to-USB adapter for flashing)
* Cable with one RJ45 plug and just wires on the other end. 1-8: A-, B+, nc, nc, Gnd, Gnd, 5V, 5V
```
    __
 __/__\__
 |front |
 ||||||||
 1 ...  8
 ABxx--++
```

Comments welcome
Joachim Banzhaf


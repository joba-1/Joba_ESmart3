# ESP32 Arduino Firmware to Toggle Load Output of an eSmart-40A with the Joba_ESmart3 Library

Basic "hello world" example for the eSmart3 charge controller

# Installation
There are many options to compile and install an ESP32 Arduino firmware. I use this one on linux:
* Install MS Code
 * Install PlatformIO as MS Code extension
* Add this folder to the MS Code workspace
* Edit platformio.ini in this folder so the usb device name for upload matches your environment
* Select build and upload the firmware

# Connection
See library readme for wiring details. 

Required hardware:
* A MAX485 board (or similar) to convert RS485 signals to serial.
* A level shifter from 5V to 3.3V (at least 3 channels) if the MAX485 board does not already have one
* An ESP32 (with USB connection or an additional serial-to-USB adapter for flashing)
* Cable with one RJ45 plug and just wires on the other end. 1-8: A-, B+, nc, nc, Gnd, Gnd, 5V, 5V
* eSmart3 40A (or similar)
* 12-48V battery attached to BAT+/- of the eSmart3
* Some load (matching battery voltage) attached to LOAD+/- 
```
    __
 __/__\__
 |front |
 ||||||||
 1 ...  8
 ABxx--++
```

# Result
The load should be toggled on and off every 10s. The LCD display of the eSmart3 will show it.


Comments welcome

Joachim Banzhaf

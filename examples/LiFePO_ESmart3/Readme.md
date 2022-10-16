# ESP32 Arduino Firmware to Setup and Monitor eSmart-40A for a 4s 272Ah LiFePO battery with the Joba_ESmart3 Library

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
See library readme for wiring details. 

Required hardware:
* A MAX485 board (or similar) to convert RS485 signals to serial.
* A level shifter from 5V to 3.3V (at least 3 channels) if the MAX485 board does not already have one
* An ESP32 (with USB connection or an additional serial-to-USB adapter for flashing)
* Cable with one RJ45 plug and just wires on the other end. 1-8: A-, B+, nc, nc, Gnd, Gnd, 5V, 5V
* eSmart3 40A (or adapt 40A limit)
* LiFePO battery with 272 Ah (or adapt 1C limit)
```
    __
 __/__\__
 |front |
 ||||||||
 1 ...  8
 ABxx--++
```

# Output
This is how the output of the program looks like after a reset and a successful connection to the eSmart3

```
Start LiFePO_ESmart3 1.0
setBatParam done
setProParam done

getChgSts ChgMode: 0, PvVolt: 1/10 V, BatVolt: 141/10 V, ChgCurr: 0/10 A, OutVolt: 0/10 V, LoadVolt: 0/10 V, LoadCurr: 0/10 A, ChgPower: 0 W, LoadPower: 0 W, BatTemp: 24 °C, InnerTemp: 25 °C, BatCap: 100 %, CO2: 1900544/10 kg, Fault: 00-00-00-10-00, SystemReminder: 0
getBatParam BatType: 0, BatSysType: 1, BulkVolt: 140/10 V, FloatVolt: 0/10 V, MaxChgCurr: 400/10 A, MaxDisChgCurr: 400/10 A, EqualizeChgVolt: 144/10 V, EqualizeChgTime: 90 min, LoadUseSel: 0 %, ChkSum: x0001, Flag: x4442
getProParam LoadOvp: 150/10 V, LoadUvp: 124/10 V, BatOvp: 163/10 V, BatOvB (Recov): 149/10 V, BatUvp: 112/10 V, BatUvB (Recov): 119/10 V, ChkSum: x3ffb, Flag: x4440
getInformation model: eSmart3-40A-MPPT, serial: 34000083, date: 20220225, firmware: V4.2, flags: x4442, checksum: x94e0
```


Comments welcome

Joachim Banzhaf


# ESP32 Arduino Firmware to Monitor eSmart-40A and send Status to Influx DB with the Joba_ESmart3 Library

Work in Progress: post to influx not working yet (hangs after first insert)

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
* eSmart3 MPPT charge controller
* LiFePO battery
```
    __
 __/__\__
 |front |
 ||||||||
 1 ...  8
 ABxx--++
```

# InfluxDB
relevant connection data (Influx host, database, ...) is configured in platformio.ini

* checks Information at startup
* checks ChgSts every second
* checks BatParam, LoadParam, ProParam every minute
* checks Log(wStartCnt, wFaultCnt, dwTotalEng, dwLoadTotalEng, wBacklightTime, bSwitchEnable) every minute  
* updates database at startup and on changes


# Networking
since WiFi is needed for Influx anyways, it is used for other stuff as well:
* Webserver 
    * display status page of ChgSts (meta refresh or ajax)
    * OTA firmware update
    * display and update of some values of BatParam, LoadParam, ProParam and Log
* NTP to set ESmart3 time if out of sync (read ESmart time needed) or at startup once
* Syslog and mqtt publish of status on changes (max once per minute)


Comments welcome

Joachim Banzhaf
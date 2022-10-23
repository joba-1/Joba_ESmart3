# Monitor for eSmart3 MPPT solar charger

ESP32 Arduino firmware to monitor eSmart3 chargers and send status to an Influx db with the Joba_ESmart3 library.
Switching load on and off is also possible.

Running the monitor on an ESP8266 might work but is not tested.

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
Create necessary database like this on the influx server: `influx --execute 'create database eSmart3'` 

* checks Information every ten minutes
* checks ChgSts every half second
* checks BatParam, LoadParam, ProParam every minute
* checks Log(wStartCnt, wFaultCnt, dwTotalEng, dwLoadTotalEng, wBacklightTime, bSwitchEnable) every minute  
* updates database at startup and on changes


# Networking
since WiFi is needed for Influx anyways, it is used for other stuff as well:
* Webserver 
    * display links for JSON of all item categories
    * enables OTA firmware update
    * display (and later update) of some values of BatParam, LoadParam, ProParam and Log
* planned: NTP to set ESmart3 time if out of sync (maybe later: read ESmart time needed) or at startup once
* Syslog (and later mqtt publish) of status on changes


Comments welcome

Joachim Banzhaf

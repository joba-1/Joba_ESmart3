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
# Example Output
Some output from the firmware to see what can be read and written
```
execute ChgSts command: 3, len: 28, Ubat: 112/10V
getChgSts partial Ubat: 112/10 V, Tbat: 20 °C, Fault: x0040
getChgSts ChgMode: 0, PvVolt: 1/10 V, BatVolt: 112/10 V, ChgCurr: 0/10 A, OutVolt: 0/10 V, LoadVolt: 0/10 V, LoadCurr: 0/10 A, ChgPower: 0 W, LoadPower: 0 W, BatTemp: 20 °C, InnerTemp: 22 °C, BatCap: 30 %, CO2: 1900544/10 kg, Fault: 00-00-00-10-00, SystemReminder: 0
getBatParam BatType: 0, BatSysType: 1, BulkVolt: 140/10 V, FloatVolt: 128/10 V, MaxChgCurr: 0/10 A, MaxDisChgCurr: 0/10 A, EqualizeChgVolt: 144/10 V, EqualizeChgTime: 20 min, LoadUseSel: 65535 %, ChkSum: x0001, Flag: x0000
setBatParam done
getLog RunTime: 13107200 min, StartCnt: 1, LastFaultInfo: x13, FaultCnt: 0, TodayEng: 0 Wh, TodayEngDate m-d: 0-0, MonthEng: 0 Wh, MonthEngDate m-d: 0-0, TotalEng: 2031616 Wh, LoadTodayEng: 0 Wh, LoadMonthEng: 0 Wh, LoadTotalEng: 0 Wh, BacklightTime: 15 s, SwitchEnable: 0, ChkSum: x454c, Flag: x4442
getParameters wPvVoltRatio: 3263, PvVoltOffset: 0, BatVoltRatio: 1615, BatVoltOffset: 0, ChgCurrRatio: 1096, ChgCurrOffset: 145, LoadCurrRatio: 1139, LoadCurrOffset: 140, LoadVoltRatio: 1628, LoadVoltOffset: 0, OutVoltRatio: 1666, OutVoltOffset: 0, ChkSum: x6e03, Flag: x4440
getLoadParam LoadModuleSelect1: 5118, LoadModuleSelect2: 5200, LoadOnPvVolt: 300/10 V, LoadOffPvVolt: 500/10 V, PvContrlTurnOnDelay: 10 min, PvContrlTurnOffDelay: 10 min, AftLoadOnTime h.m: 18.30, AftLoadOffTime h.m: 21.30, MonLoadOnTime h.m: 5.10, MonLoadOffTime h.m: 6.30, LoadSts: 0, Time2Enable: 1, ChkSum: x7058, Flag: x4440
getProParam LoadOvp: 160/10 V, LoadUvp: 105/10 V, BatOvp: 160/10 V, BatOvB (Recov): 150/10 V, BatUvp: 105/10 V, BatUvB (Recov): 110/10 V, ChkSum: x4756, Flag: x4440
getInformation model: eSmart3-40A-MPPT, serial: 34000083, date: 20220225, firmware: V4.2, flags: x4442, checksum: x94e0
getEngSave MonthLoadPower month 1: 0 Wh, month 2: 0 Wh, month 3: 0 Wh, month 4: 0 Wh, month 5: 0 Wh, month 6: 0 Wh, month 7: 0 Wh, month 8: 0 Wh, month 9: 0 Wh, month 10: 0 Wh, month 11: 0 Wh, month 12: 0 Wh
setBacklightTime: 14 s
setMaxLoadCurrent: 10 A
setMaxChargeCurrent: 20 A
setDisplayTemperatureUnit: F
getDisplayTemperatureUnit: Fahrenheit
setLoad: ON
setSwitchEnable: ON
```
Comments welcome
Joachim Banzhaf


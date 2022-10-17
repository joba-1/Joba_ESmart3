#ifndef ESMART3
#define ESMART3

/* 
Class to communicate with an eSmart3 MPPT charge controller via RS485

The device organizes data in item groups. 
Groups can be read with GET command and written with SET command
The device answers with ACK (ok) or NACK (error)
Groups can be read and written as a whole or in parts definded by [start, end[ word offsets
The 1-byte crc of the device is handled internally by the lib, as well as lengths and offsets in the data region. 
Except for the basic execute method the command header is also maintained by the lib.
The execute method is only needed for functions not implemented by higher level get/set-methods

Used names are from Feb 2018 MPPT Solar Controller communication protocol document (only some "typos" fixed)
Notes: 
* Assumes endianness LE (ok for e.g. ESP32, ESP8266, RPi, x86)
* Not every detail is tested, only what I needed for my setup. Bug reports welcome.
* Controlling load output (by timer/light) is not implemented, for now this needs to be done by execute method.
* setTime() works, but without ESP-reboot communication gets unreliable, especially when called more than once.
* Different wirings are possible. eSmart3 has 5V logic levels and ESP32 uses 3.3V. 
  Some RS485 boards have builtin 5V<->3.3V level shifters and automatic direction selection (recommended).
  Other RS485 boards (like mine) have only 5V DI/RO and use DE/!RE for manual direction selection. 
  For those boards a level shifter for 3 lines is needed (I used a TXS108E/YF08E)
* The RS485 board (and level shifter) needs a 5V supply. The eSmart3 5V from the RS485 port can be used.
  But these 5V are too weak for powering ESP32 boards (at least with WLAN enabled).

Example wiring with level shifter
            ____________
_____      |         !RE|--+   _____________       __________
e    |     |          DE|--+--|B3         A3|-----|IO22      |
S  A-|-----|A-        DI|-----|B2         A2|-----|RX2       |
m  B+|-----|B+ MAX485 RO|-----|B1 TXB0108 A1|-----|TX2 ESP32 |    _________     ____
a  5V|-----|VCC---------------|VCCB     VCCA|--+--|3.3V   TX1|---|RX  USB- |   | PC |
r GND|--+--|GND_________|  +--|GND________OE|--+  |       RX1|---|TX Serial|===|USB |
t    |  +------------------+--------------------+-|GND____Vin|---|VCC      |   |____|
3____|                                          +----------------|GND______|

Author: Joachim.Banzhaf@gmail.com
License: GPL V2
*/

#include <Arduino.h>
#include <Stream.h>
#include <time.h>

// Don't use padding in structures to match what ESmart3 devices need
#pragma pack(2)

class ESmart3 {
public:
    // Datatypes used by the device

    typedef struct header {
        uint8_t start, device, address, command, item, length;
    } header_t;

    typedef enum dev { ALL, MPPT } dev_t;
    typedef enum addr { BROADCAST } addr_t;

    typedef enum cmd {
        ACK,
        GET,
        SET,
        SET_NO_RESP,
        NACK,
        EXEC,
        ERR = 0x7f
    } cmd_t;

    typedef enum item {
        ChgSts,
        BatParam,
        Log,
        Parameters,
        LoadParam,
        ChgDebug,
        RemoteControl,
        ProParam,
        Information,
        TempParam,
        EngSave
    } item_t;

    typedef enum chgMode { CHG_WAIT, CHG_MPPT, CHG_BULK, CHG_FLOAT, CHG_PRE } chgMode_t;

    typedef struct ChgSts {
        uint16_t wChgMode;  // see chgMode_t
        uint16_t wPvVolt;
        uint16_t wBatVolt;
        uint16_t wChgCurr;
        uint16_t wOutVolt;
        uint16_t wLoadVolt;
        uint16_t wLoadCurr;
        uint16_t wChgPower;
        uint16_t wLoadPower;
        int16_t wBatTemp;
        int16_t wInnerTemp;
        uint16_t wBatCap;
        uint32_t dwCO2;
        uint16_t wFault;
        uint16_t wSystemReminder;
    } ChgSts_t;

    typedef struct BatParam {
        uint16_t wFlag;  // TODO what is this?
        uint16_t wBatType;
        uint16_t wBatSysType;
        uint16_t wBulkVolt;  // normal charge voltage (13.9V for lead-acid)
        uint16_t wFloatVolt;  // storage voltage after full charge (13.3V for lead-acid)
        uint16_t wMaxChgCurr;
        uint16_t wMaxDisChgCurr;  // WARNING: does not limit current, only sets fault flag
        uint16_t wEqualizeChgVolt;  // voltage to compensate storage loss (14.4V for lead-acid)
        uint16_t wEqualizeChgTime;  // ~15-30 min every day for lead-acid
        uint16_t bLoadUseSel;
        uint16_t ChkSum;  // TODO currently ignored
    } BatParam_t;

    typedef struct {
        int16_t month;
        int16_t day;
    } data_t;

    typedef struct Log {
        uint16_t wFlag;
        uint32_t dwRunTime;
        uint16_t wStartCnt;
        uint16_t wLastFaultInfo;
        uint16_t wFaultCnt;
        uint32_t dwTodayEng;
        data_t wTodayEngDate;
        uint32_t dwMonthEng;
        data_t wMonthEngDate;
        uint32_t dwTotalEng;
        uint32_t dwLoadTodayEng;
        uint32_t dwLoadMonthEng;
        uint32_t dwLoadTotalEng;
        uint16_t wBacklightTime;
        uint16_t bSwitchEnable;
        uint16_t ChkSum;
    } Log_t;

    typedef struct Parameters {
        uint16_t wFlag;
        uint16_t wPvVoltRatio;
        uint16_t wPvVoltOffset;
        uint16_t wBatVoltRatio;
        uint16_t wBatVoltOffset;
        uint16_t wChgCurrRatio;
        uint16_t wChgCurrOffset;
        uint16_t wLoadCurrRatio;
        uint16_t wLoadCurrOffset;
        uint16_t wLoadVoltRatio;
        uint16_t wLoadVoltOffset;
        uint16_t wOutVoltRatio;
        uint16_t wOutVoltOffset;
        uint16_t ChkSum;
    } Parameters_t;

    typedef struct {
        int16_t hour;
        int16_t minute;
    } time_t;

    typedef struct LoadParam {
        uint16_t wFlag;
        uint16_t wLoadModuleSelect1;
        uint16_t wLoadModuleSelect2;
        uint16_t wLoadOnPvVolt;
        uint16_t wLoadOffPvVolt;
        uint16_t wPvContrlTurnOnDelay;
        uint16_t wPvContrlTurnOffDelay;
        time_t AftLoadOnTime;
        time_t AftLoadOffTime;
        time_t MonLoadOnTime;
        time_t MonLoadOffTime;
        uint16_t wLoadSts;
        uint16_t wTime2Enable;
        uint16_t ChkSum;
    } LoadParam_t;

    typedef struct RemoteControl {
        int16_t uwMagicNum;
        int16_t eRemoteCmd;
        int16_t uwData[8];
    } RemoteControl_t;

    typedef struct ProParam {
        uint16_t wFlag;
        uint16_t wLoadOvp;
        uint16_t wLoadUvp;
        uint16_t wBatOvp;
        uint16_t wBatOvB;
        uint16_t wBatUvp;
        uint16_t wBatUvB;
        uint16_t ChkSum;
    } ProParam_t;

    typedef struct Information {
        uint16_t wFlag;
        union {
            uint16_t wSerialID[8];    // not null terminated
            struct { 
                uint16_t wSerial[4];  // not null terminated
                uint16_t wDate[4];    // not null terminated
            };
        };
        uint16_t wFirmWare[2];        // not null terminated
        uint16_t wModel[8];           // not null terminated
        uint16_t ChkSum;
    } Information_t;

    typedef enum tempUnit { CELSIUS, FAHRENHEIT } tempUnit_t;

    typedef struct TempParam {
        uint16_t wFlag;
        time_t PeriodLoadCtr2Time;
        uint16_t bPVEnergySel;
        uint16_t bBatTempSel;
        uint16_t bLoadUseSel;
        uint16_t bLoadLaststate;
        uint16_t ChkSum;
    } TempParam_t;

    typedef struct EngSave {
        uint16_t wFlag;
        uint32_t wMonthPower[12];
        uint32_t wMonthLoadPower[12];
        uint32_t wDayPower[31];
        uint32_t wDayLoadPower[31];
        uint16_t ChkSum;
    } EngSave_t;


    // Basic methods

    // Object represents device at serial port. Send commands with minimal delay given 
    ESmart3( Stream &serial, uint8_t command_delay_ms = 10 );

    // Init serial interface. Set dir_pin to -1 if RS485 hardware sets direction automatically
    void begin( int dir_pin = -1 );

    // Send header and command then receive header and result (not including offset or crc)
    // Return true if header and command are written and result and header are read successfully
    bool execute( header_t &header, uint8_t *command, uint8_t *result );


    // Get-Commands. If [start, end[ is given (in 16bit offset steps from manual), only relevant part of data is used
    // Return true if execute() was successful

    bool getChgSts( ChgSts_t &data, size_t start = 0, size_t end = sizeof(ChgSts_t) / 2 );
    bool getBatParam( BatParam_t &data, size_t start = 0, size_t end = sizeof(BatParam_t) / 2 );
    bool getLog( Log_t &data, size_t start = 0, size_t end = sizeof(Log_t) / 2 );
    bool getParameters( Parameters_t &data, size_t start = 0, size_t end = sizeof(Parameters_t) / 2 );
    bool getLoadParam( LoadParam_t &data, size_t start = 0, size_t end = sizeof(LoadParam_t) / 2 );
    bool getProParam( ProParam_t &data, size_t start = 0, size_t end = sizeof(ProParam_t) / 2 );
    bool getInformation( Information_t &data, size_t start = 0, size_t end = sizeof(Information_t) / 2 );
    bool getEngSave( EngSave_t &data, size_t start = 0, size_t end = sizeof(EngSave_t) / 2 );

    bool getLoad( bool &on );
    bool getDisplayTemperatureUnit( tempUnit_t &unit );


    // Set-Commands

    // Set BatParam_t values in data between [start, end[ word offsets (default excludes wFlag, bLoadUseSel and ChkSum)
    // Factory default on mine was
    //   BatType: user, BatSysType: auto, BulkVolt: 14,4V, FloatVolt: 13,7V, 
    //   MaxChgCurr: 40A, MaxDisChgCurr: 40A, EqualizeChgVolt: 14,6V, EqualizeChgTime: 30min
    bool setBatParam( BatParam_t &data, size_t start = 1, size_t end = sizeof(BatParam_t) / 2 - 2 );
    // Set ProParam_t values in data between [start, end[ word offsets (default excludes wFlag and ChkSum)
    // Factory default on mine was
    ///  LoadOvp: 16V, LoadUvp: 10,5V, BatOvp: 16V, BatOvB 15V, BatUvp: 10,5V, BatUvB (Recov): 11V
    bool setProParam( ProParam_t &data, size_t start = 1, size_t end = sizeof(ProParam_t) / 2 - 1 );

    bool setMaxChargeCurrent( uint16_t deciAmps );
    bool setMaxLoadCurrent( uint16_t deciAmps );
    bool setBacklightTime( uint16_t sec );
    bool setLoad( bool on );
    bool setTime( struct tm &now );  // TODO works only once?
    bool setDisplayTemperatureUnit( tempUnit_t unit );  // Only affects display. ChgSts_t always uses °C
    bool setSwitchEnable( bool on );  // TODO seems to work, but what is this?


    // Static helper functions

    static bool isBatteryVoltageOver( uint16_t fault )        { return fault & 0x001; };
    static bool isPvVoltageOver( uint16_t fault )             { return fault & 0x002; };
    static bool isChargeCurrentOver( uint16_t fault )         { return fault & 0x004; };
    static bool isDischargeCurrentOver( uint16_t fault )      { return fault & 0x008; };
    static bool isBatteryTemperatureAlarm( uint16_t fault )   { return fault & 0x010; };
    static bool isInternalTemperatureAlarm( uint16_t fault )  { return fault & 0x020; };
    static bool isPvVoltageLow( uint16_t fault )              { return fault & 0x040; };
    static bool isBatteryVoltageLow( uint16_t fault )         { return fault & 0x080; };
    static bool isTripZeroProtectionTrigger( uint16_t fault ) { return fault & 0x100; };
    static bool isControlByManualSwitchgear( uint16_t fault ) { return fault & 0x200; };

private:
    uint8_t genCrc( header_t &header, uint8_t *offset, uint8_t *data );
    bool isValid( header_t &header, uint8_t *offset, uint8_t *data, uint8_t crc );
    bool prepareCmd( header_t &header, uint8_t *command, uint8_t &crc );
    uint8_t *initGetOffset( uint8_t *cmd, uint8_t *data, size_t start, size_t end );
    void initSetOffset( uint8_t *cmd, uint8_t *data, size_t start, size_t end );

    Stream &_serial;
    uint8_t _delay;
    uint32_t _prev;
    int _dir_pin;
};

#endif

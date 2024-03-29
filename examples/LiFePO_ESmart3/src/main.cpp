/*
LiFePO Setup Program for eSmart3 via RS485 communication

WARNING: Use the values as defined in setup() at your own risk! They should be ok, but are currently untested.

Monitors GPIO 0 pulled to ground as key press to toggle load
Use builtin led to represent load status
*/


#include <Arduino.h>

#include <esmart3.h>

#if defined(ESP8266)
#include <SoftwareSerial.h>
SoftwareSerial Serial2;  // Use SoftwareSerial port
#define LED_ON LOW
#define LED_OFF HIGH
#elif defined(ESP32)
// Serial2 already defined
#define LED_ON HIGH
#define LED_OFF LOW
#else
// define Serial2 as your non ESP serial port here
#endif

#define LED_PIN LED_BUILTIN
#define BTN_PIN 0


ESmart3 esmart3(Serial2);  // Use ESP32 HardwareSerial port Serial2 to commiunicate with RS485 adapter


void setup() {
    Serial.begin(115200);

#if defined(ESP8266)
    Serial2.begin(9600, SWSERIAL_8N1, 13, 15);  // Use pins 13 and 15 for RX and TX
#elif defined(ESP32)
    Serial2.begin(9600, SERIAL_8N1);  // Use Serial2 default pins 16 and 17 for RX and TX
#else
    // init your non ESP serial port here
#endif

    pinMode(LED_PIN, OUTPUT);  // to show load status
    pinMode(BTN_PIN, INPUT_PULLUP);  // to toggle load status

    esmart3.begin(22);  // Use pin 22 for explicit DE/!RE
    
    Serial.println("\nStart " PROGNAME " " VERSION );

    uint16_t s_cells = 4;  // 4, 8, 12, 16
    uint16_t p_cells = 1;  // 1 - 4
    uint16_t maxCellDeciVolt = 36;  // ~95% LiFePO capacity
    uint16_t minCellDeciVolt = 31;  // ~15% LiFePO capacity
    uint16_t capacityAh = 272;  // my LiFePO capacity
    uint16_t maxDeviceCurr = 400;  // max deciAmps of my eSmart3
    
    ESmart3::BatParam_t batParam = {0};
    batParam.wBatType = 0;  // LiFePO: user(0)
    batParam.wBatSysType = s_cells / 4;  // SysType=12V-multiplier and 4 LiFePO cells are ~12V
    batParam.wBulkVolt = maxCellDeciVolt * s_cells;  // CC -> CV limit
    batParam.wFloatVolt = 0;  // no continuous charge for lifepo
    batParam.wEqualizeChgVolt = 0;  // no monthly refresh charge for lifepo
    batParam.wEqualizeChgTime = 0;  // minutes refresh for lifepo
    batParam.wMaxChgCurr = capacityAh * p_cells * 10;  // 1C for LiFePO
    if( batParam.wMaxChgCurr > maxDeviceCurr ) {
        batParam.wMaxChgCurr = maxDeviceCurr;  // eSmart3 40A limit
    }
    batParam.wMaxDisChgCurr = batParam.wMaxChgCurr;  // same as charge for LiFePO and eSmart3
    if( esmart3.setBatParam(batParam) ) {
        Serial.println("setBatParam done");
    }
    else {
        Serial.println("setBatParam error");
    }

    ESmart3::ProParam_t proParam = {0};
    proParam.wLoadOvp = 150;  // protect end device from >= 15V
    proParam.wLoadUvp = minCellDeciVolt * s_cells;  // protect battery from load if < 15% capa
    proParam.wBatOvB = maxCellDeciVolt * s_cells + 5; // unprotect batttery at slightly above max voltage
    proParam.wBatOvp = proParam.wBatOvB + proParam.wBatOvB / 10; // protect batttery from > 10% max voltage
    proParam.wBatUvp = proParam.wLoadUvp - proParam.wLoadUvp / 10;  // protect battery 10% below wLoadUvp
    proParam.wBatUvB = proParam.wLoadUvp - 5;  // recovery slightly below wLoadUvp
    if( esmart3.setProParam(proParam) ) {
        Serial.println("setProParam done");
    }
    else {
        Serial.println("setProParam error");
    }
}


// toggle load on key press
// pin is pulled up if released and pulled down if pressed
void handle_load_button( bool loadOn ) {
    static uint32_t prevTime = 0;
    static uint16_t debounceStatus = 1;
    static bool pressed = false;

    uint32_t now = millis();
    if( now - prevTime > 2 ) {  // debounce check every 2 ms, decision after 2ms/bit * 16bit = 32ms
        prevTime = now;

        // shift bits left, set lowest bit if button pressed
        debounceStatus = (debounceStatus << 1) | ((digitalRead(BTN_PIN) == LOW) ? 1 : 0);

        if( debounceStatus == 0x0000 && pressed ) {
            pressed = false;
        }
        else if( debounceStatus == 0xffff && !pressed ) {
            pressed = true;
            esmart3.setLoad(!loadOn);
            if( !loadOn ) {
                Serial.println("Load switched ON");
            }
            else {
                Serial.println("Load switched OFF");
            }
        }
    }
}


// check once every 500ms if load status has changed
// return true if load is on or unknown
bool handle_load_led() {
    static uint32_t prevTime = 0;
    static bool prevStatus = false;  // status unknown
    static bool prevLoad = true;     // assume load is on

    uint32_t now = millis();
    if( now - prevTime > 500 ) {
        prevTime = now;
        bool loadOn = false;
        if( esmart3.getLoad(loadOn) ) {
            if( !prevStatus || loadOn != prevLoad ) {
                if( loadOn ) {
                    digitalWrite(LED_PIN, LED_ON);
                    Serial.println("Load is ON");
                }
                else {
                    digitalWrite(LED_PIN, LED_OFF);
                    Serial.println("Load is OFF");
                }
                prevStatus = true;
                prevLoad = loadOn;
            }
        }
        else {
            if( prevStatus ) {
                digitalWrite(LED_PIN, LED_ON);  // assume ON
                Serial.println("Load is UNKNOWN");
                prevStatus = false;
                prevLoad = true;
            }
        }
    }

    return prevLoad;
}


// print every 5s status of esmart3
void handle_esmart3_status() {
    static uint32_t prevTime = 0;

    uint32_t now = millis();
    if( now - prevTime > 5000 ) {
        prevTime = now;

        Serial.println();

        ESmart3::ChgSts_t stats = {0};
        if( esmart3.getChgSts(stats) ) {
            Serial.printf("getChgSts ChgMode: %u, ", stats.wChgMode);
            Serial.printf("PvVolt: %u/10 V, ", stats.wPvVolt);
            Serial.printf("BatVolt: %u/10 V, ", stats.wBatVolt);
            Serial.printf("ChgCurr: %u/10 A, ", stats.wChgCurr);
            Serial.printf("OutVolt: %u/10 V, ", stats.wOutVolt);
            Serial.printf("LoadVolt: %u/10 V, ", stats.wLoadVolt);
            Serial.printf("LoadCurr: %u/10 A, ", stats.wLoadCurr);
            Serial.printf("ChgPower: %u W, ", stats.wChgPower);
            Serial.printf("LoadPower: %u W, ", stats.wLoadPower);
            Serial.printf("BatTemp: %d °C, ", stats.wBatTemp);
            Serial.printf("InnerTemp: %d °C, ", stats.wInnerTemp);
            Serial.printf("BatCap: %u %%, ", stats.wBatCap);
            Serial.printf("CO2: %u/10 kg, ", stats.dwCO2);
            Serial.printf("Fault: %d%d-%d%d-%d%d-%d%d-%d%d, ", ESmart3::isBatteryVoltageOver(stats.wFault), ESmart3::isPvVoltageOver(stats.wFault),
                ESmart3::isChargeCurrentOver(stats.wFault), ESmart3::isDischargeCurrentOver(stats.wFault), ESmart3::isBatteryTemperatureAlarm(stats.wFault),
                ESmart3::isInternalTemperatureAlarm(stats.wFault), ESmart3::isPvVoltageLow(stats.wFault), ESmart3::isBatteryVoltageLow(stats.wFault),
                ESmart3::isTripZeroProtectionTrigger(stats.wFault), ESmart3::isControlByManualSwitchgear(stats.wFault));
            Serial.printf("SystemReminder: %u\n", stats.wSystemReminder);
        }
        else {
            Serial.println("getChgSts error");
        }

        ESmart3::BatParam_t batParam = {0};
        if( esmart3.getBatParam(batParam) ) {
            Serial.printf("getBatParam BatType: %u, ", batParam.wBatType);
            Serial.printf("BatSysType: %u, ", batParam.wBatSysType);
            Serial.printf("BulkVolt: %u/10 V, ", batParam.wBulkVolt);
            Serial.printf("FloatVolt: %u/10 V, ", batParam.wFloatVolt);
            Serial.printf("MaxChgCurr: %u/10 A, ", batParam.wMaxChgCurr);
            Serial.printf("MaxDisChgCurr: %u/10 A, ", batParam.wMaxDisChgCurr);
            Serial.printf("EqualizeChgVolt: %u/10 V, ", batParam.wEqualizeChgVolt);
            Serial.printf("EqualizeChgTime: %u min, ", batParam.wEqualizeChgTime);
            Serial.printf("LoadUseSel: %u %%, ", batParam.bLoadUseSel);
            Serial.printf("ChkSum: x%04x, ", batParam.ChkSum);
            Serial.printf("Flag: x%04x\n", batParam.wFlag);
        }
        else {
            Serial.println("getBatParam error");
        }

        ESmart3::ProParam_t proParam = {0};
        if( esmart3.getProParam(proParam) ) {
            Serial.printf("getProParam LoadOvp: %u/10 V, ", proParam.wLoadOvp);
            Serial.printf("LoadUvp: %u/10 V, ", proParam.wLoadUvp);
            Serial.printf("BatOvp: %u/10 V, ", proParam.wBatOvp);
            Serial.printf("BatOvB (Recov): %u/10 V, ", proParam.wBatOvB);
            Serial.printf("BatUvp: %u/10 V, ", proParam.wBatUvp);
            Serial.printf("BatUvB (Recov): %u/10 V, ", proParam.wBatUvB);
            Serial.printf("ChkSum: x%04x, ", proParam.ChkSum);
            Serial.printf("Flag: x%04x\n", proParam.wFlag);
        }
        else {
            Serial.println("getProParam error");
        }

        ESmart3::Information_t infos = {0};
        if( esmart3.getInformation(infos) ) {
            Serial.printf("getInformation model: %16.16s, serial: %8.8s, date: %8.8s, firmware: %4.4s, flags: x%04x, checksum: x%04x\n",
                (char *)infos.wModel, (char *)infos.wSerial, (char *)infos.wDate, (char *)infos.wFirmWare, infos.wFlag, infos.ChkSum);
        }
        else {
            Serial.println("getInformation error");
        }
    }
}


void loop() {
    handle_load_button(handle_load_led());
    handle_esmart3_status();
}

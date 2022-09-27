/*
Test Program for eSmart3 RS485 communication
*/

#include <Arduino.h>

#include <esmart3.h>

ESmart3 esmart3(Serial2);  // Use ESP32 HardwareSerial port Serial2 to commiunicate with RS485 adapter

void setup() {
    Serial.begin(115200);
    esmart3.begin(-1, -1, 22);  // Use Serial2 default pins 16 and 17 for RX and TX and pin 22 for explicit DE/!RE
    Serial.println("\nStart " PROGNAME " " VERSION );
}

void loop() {
    static uint8_t count = 0;

    if( ++count > 3 ) count = 0;
    uint8_t command[] = { 0x00, 0x00, 0x1a };
    ESmart3::header_t header = { 0xaa, 0x01, 0x00, 0x01, 0x00, sizeof(command) };
    uint8_t result[120] = { 0 };
    // struct tm now = { 0 };
    // now.tm_year = 2022;
    // now.tm_mon = 9;
    // now.tm_mday = 27;
    // now.tm_hour = 2;
    // now.tm_min = 53;
    // now.tm_sec = 44;

    Serial.println();
    if( esmart3.execute(header, command, result) ) {
        uint16_t *uBat = (uint16_t *)&result[4];
        Serial.printf("execute ChgSts command: %u, len: %u, Ubat: %u/10V\n", header.command, header.length, *uBat);

        ESmart3::ChgSts_t stats = {0};
        if( esmart3.getChgSts(stats, 2, 0x0f) ) {
            char warn[] = "\n\nWARNING:\n\n";
            Serial.printf("%sgetChgSts partial Ubat: %u/10 V, Tbat: %u °C, Fault: x%04x\n", 
                ESmart3::isBatteryVoltageLow(stats.wFault) ? warn : "", stats.wBatVolt, stats.wBatTemp, stats.wFault);
        }
        else {
            Serial.println("getChgSts partial error");
        }

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

            uint16_t s_cells = (count + 1) * 4;  // 4, 8, 12, 16
            uint16_t p_cells = count + 1;  // 1 - 4
            uint16_t maxCellDeciVolt = 36;  // ~90% LiFePO capacity
            uint16_t minCellDeciVolt = 30;  // ~20% LiFePO capacity
            uint16_t capacityAh[] = { 100, 200, 280, 320 };  // common LiFePO capacities
            ESmart3::BatParam_t batParam2 = {0};
            batParam2.wBatType = count;  // LiFePO: user(0)
            batParam2.wBatSysType = s_cells / 4;  // SysType=12V-multiplier and 4 LiFePO cells are ~12V
            batParam2.wBulkVolt = (maxCellDeciVolt - 1) * s_cells;
            batParam2.wFloatVolt = (minCellDeciVolt + 2) * s_cells;
            batParam2.wEqualizeChgVolt = maxCellDeciVolt * s_cells;
            batParam2.wEqualizeChgTime = (count + 2) * 10;  // 20, 30, 40, 50 min
            batParam2.wMaxChgCurr = capacityAh[count] * p_cells * 10;  // 1C for LiFePO
            batParam2.wMaxDisChgCurr = batParam.wMaxChgCurr;  // same for LiFePO
            if( esmart3.setBatParam(batParam2) ) {
                Serial.println("setBatParam done");
            }
            else {
                Serial.println("setBatParam error");
            }
        }
        else {
            Serial.println("getBatParam error");
        }

        ESmart3::Log_t log = {0};
        if( esmart3.getLog(log) ) {
            Serial.printf("getLog RunTime: %u min, ", log.dwRunTime);
            Serial.printf("StartCnt: %u, ", log.wStartCnt);
            Serial.printf("LastFaultInfo: x%x, ", log.wLastFaultInfo);
            Serial.printf("FaultCnt: %u, ", log.wFaultCnt);
            Serial.printf("TodayEng: %u Wh, ", log.dwTodayEng);
            Serial.printf("TodayEngDate m-d: %u-%u, ", log.wTodayEngDate.month, log.wTodayEngDate.day);
            Serial.printf("MonthEng: %u Wh, ", log.dwMonthEng);
            Serial.printf("MonthEngDate m-d: %u-%u, ", log.wMonthEngDate.month, log.wMonthEngDate.day);
            Serial.printf("TotalEng: %u Wh, ", log.dwTotalEng);
            Serial.printf("LoadTodayEng: %u Wh, ", log.dwLoadTodayEng);
            Serial.printf("LoadMonthEng: %u Wh, ", log.dwLoadMonthEng);
            Serial.printf("LoadTotalEng: %u Wh, ", log.dwLoadTotalEng);
            Serial.printf("BacklightTime: %u s, ", log.wBacklightTime);
            Serial.printf("SwitchEnable: %u, ", log.bSwitchEnable);
            Serial.printf("ChkSum: x%04x, ", log.ChkSum);
            Serial.printf("Flag: x%04x\n", log.wFlag);
        }
        else {
            Serial.println("getLog error");
        }

        ESmart3::Parameters_t param = {0};
        if( esmart3.getParameters(param) ) {
            Serial.printf("getParameters wPvVoltRatio: %u, ", param.wPvVoltRatio);
            Serial.printf("PvVoltOffset: %u, ", param.wPvVoltOffset);
            Serial.printf("BatVoltRatio: %u, ", param.wBatVoltRatio);
            Serial.printf("BatVoltOffset: %u, ", param.wBatVoltOffset);
            Serial.printf("ChgCurrRatio: %u, ", param.wChgCurrRatio);
            Serial.printf("ChgCurrOffset: %u, ", param.wChgCurrOffset);
            Serial.printf("LoadCurrRatio: %u, ", param.wLoadCurrRatio);
            Serial.printf("LoadCurrOffset: %u, ", param.wLoadCurrOffset);
            Serial.printf("LoadVoltRatio: %u, ", param.wLoadVoltRatio);
            Serial.printf("LoadVoltOffset: %u, ", param.wLoadVoltOffset);
            Serial.printf("OutVoltRatio: %u, ", param.wOutVoltRatio);
            Serial.printf("OutVoltOffset: %u, ", param.wOutVoltOffset);
            Serial.printf("ChkSum: x%04x, ", param.ChkSum);
            Serial.printf("Flag: x%04x\n", param.wFlag);
        }
        else {
            Serial.println("getParameters error");
        }

        ESmart3::LoadParam_t loadParam = {0};
        if( esmart3.getLoadParam(loadParam) ) {
            Serial.printf("getLoadParam LoadModuleSelect1: %u, ", loadParam.wLoadModuleSelect1);
            Serial.printf("LoadModuleSelect2: %u, ", loadParam.wLoadModuleSelect2);
            Serial.printf("LoadOnPvVolt: %u/10 V, ", loadParam.wLoadOnPvVolt);
            Serial.printf("LoadOffPvVolt: %u/10 V, ", loadParam.wLoadOffPvVolt);
            Serial.printf("PvContrlTurnOnDelay: %u min, ", loadParam.wPvContrlTurnOnDelay);
            Serial.printf("PvContrlTurnOffDelay: %u min, ", loadParam.wPvContrlTurnOffDelay);
            Serial.printf("AftLoadOnTime h.m: %u.%u, ", loadParam.AftLoadOnTime.hour, loadParam.AftLoadOnTime.minute);
            Serial.printf("AftLoadOffTime h.m: %u.%u, ", loadParam.AftLoadOffTime.hour, loadParam.AftLoadOffTime.minute);
            Serial.printf("MonLoadOnTime h.m: %u.%u, ", loadParam.MonLoadOnTime.hour, loadParam.MonLoadOnTime.minute);
            Serial.printf("MonLoadOffTime h.m: %u.%u, ", loadParam.MonLoadOffTime.hour, loadParam.MonLoadOffTime.minute);
            Serial.printf("LoadSts: %u, ", loadParam.wLoadSts);
            Serial.printf("Time2Enable: %u, ", loadParam.wTime2Enable);
            Serial.printf("ChkSum: x%04x, ", loadParam.ChkSum);
            Serial.printf("Flag: x%04x\n", loadParam.wFlag);
        }
        else {
            Serial.println("getLoadParam error");
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

        ESmart3::EngSave_t eng = {0};
        if( esmart3.getEngSave(eng, 0x19, 0x31) ) {
            Serial.print("getEngSave MonthLoadPower "); 
            for( size_t i = 0; i < 11; i++ ) {
                Serial.printf("month %u: %d Wh, ", i+1, eng.wMonthLoadPower[i]);
            } 
            Serial.printf("month 12: %d Wh\n", eng.wMonthLoadPower[11]);
        }
        else {
            Serial.println("getEngSave error");
        }

        if( esmart3.setBacklightTime(15 - count) ) {
            Serial.printf("setBacklightTime: %u s\n", 15 - count);
        }
        else {
            Serial.println("setBacklightTime error");
        }

        if( esmart3.setMaxLoadCurrent(100 * count) ) {
            Serial.printf("setMaxLoadCurrent: %u A\n", 10 * count);
        }
        else {
            Serial.println("setMaxLoadCurrent error");
        }

        if( esmart3.setMaxChargeCurrent(200 * count) ) {
            Serial.printf("setMaxChargeCurrent: %u A\n", 20 * count);
        }
        else {
            Serial.println("setMaxChargeCurrent error");
        }

        if( esmart3.setDisplayTemperatureUnit(count & 1 ? ESmart3::FAHRENHEIT : ESmart3::CELSIUS) ) {
            Serial.printf("setDisplayTemperatureUnit: %c\n", count & 1 ? 'F' : 'C');
            ESmart3::tempUnit_t unit;
            if( esmart3.getDisplayTemperatureUnit(unit) ) {
                Serial.printf("getDisplayTemperatureUnit: %s\n", unit == ESmart3::CELSIUS ? "Celsius" : "Fahrenheit");
            }
            else {
                Serial.println("getDisplayTemperatureUnit error");
            }
        }
        else {
            Serial.println("setDisplayTemperatureUnit error");
        }

        // static bool timeSet = false;
        // if( !timeSet ) {
        //     if( esmart3.setTime(now) ) {
        //         Serial.printf("setTime: %u\n", count);
        //         timeSet = true;
        //     }
        //     else {
        //         Serial.println("setTime error");
        //     }
        // }

        if( esmart3.setLoad(count < 2) ) {
            Serial.printf("setLoad: %s\n", count < 2 ? "ON" : "OFF");
        }
        else {
            Serial.println("setLoad error");
        }

        if( esmart3.setSwitchEnable(count > 0) ) {
            Serial.printf("setSwitchEnable: %s\n", count > 0 ? "ON" : "OFF");
        }
        else {
            Serial.println("setSwitchEnable error");
        }
    }
    else {
        Serial.println("execute error");
    }

    delay(5000);
}

/*
Monitor an eSmart3 via RS485 communication

Influx DB can be created with command influx -execute "create database esmart3"

Monitors GPIO 0 pulled to ground as key press to toggle load
Use builtin led to represent health status
Use defined pin LED to represent load status
Use pin 22 to toggle RS485 read/write
*/


#include <Arduino.h>

// Config for ESP8266 or ESP32
#if defined(ESP8266)
    #include <SoftwareSerial.h>
    SoftwareSerial rs485;  // Use SoftwareSerial port

    #define HEALTH_LED_ON LOW
    #define HEALTH_LED_OFF HIGH
    #define HEALTH_LED_PIN LED_BUILTIN
    #define LOAD_LED_ON HIGH
    #define LOAD_LED_OFF LOW
    #define LOAD_LED_PIN D5
    #define LOAD_BUTTON_PIN 0

    // Web Updater
    #include <ESP8266HTTPUpdateServer.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
    #include <WiFiClient.h>
    #define WebServer ESP8266WebServer
    #define HTTPUpdateServer ESP8266HTTPUpdateServer

    // Post to InfluxDB
    #include <ESP8266HTTPClient.h>

    // Time sync
    #include <NTPClient.h>
    #include <WiFiUdp.h>
    WiFiUDP ntpUDP;
    NTPClient ntp(ntpUDP, NTP_SERVER);
#elif defined(ESP32)
    HardwareSerial &rs485 = Serial2;

    #define HEALTH_LED_ON HIGH
    #define HEALTH_LED_OFF LOW
    #define HEALTH_LED_PIN LED_BUILTIN
    #define HEALTH_PWM_CH 0
    #define LOAD_LED_ON HIGH
    #define LOAD_LED_OFF LOW
    #define LOAD_LED_PIN 5
    #define LOAD_BUTTON_PIN 0

    // Web Updater
    #include <HTTPUpdateServer.h>
    #include <WebServer.h>
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #include <WiFiClient.h>

    // Post to InfluxDB
    #include <HTTPClient.h>
    
    // Time sync
    #include <time.h>
#else
    #error "No ESP8266 or ESP32, define your rs485 stream, pins and includes here!"
#endif

// Infrastructure
#include <Syslog.h>
#include <WiFiManager.h>

// Web status page and OTA updater
#define WEBSERVER_PORT 80

WebServer web_server(WEBSERVER_PORT);
HTTPUpdateServer esp_updater;

// Post to InfluxDB
WiFiClient client;
HTTPClient http;
int influx_status = 0;
time_t post_time = 0;

// Breathing status LED
const uint32_t ok_interval = 5000;
const uint32_t err_interval = 1000;

uint32_t breathe_interval = ok_interval; // ms for one led breathe cycle
bool enabledBreathing = true;  // global flag to switch breathing animation on or off

#ifndef PWMRANGE
#define PWMRANGE 1023
#define PWMBITS 10
#endif

// Syslog
WiFiUDP logUDP;
Syslog syslog(logUDP, SYSLOG_PROTO_IETF);
char msg[512];  // one buffer for all syslog and json messages
char start_time[30];

// eSmart3 device
#include <esmart3.h>

#define RS485_DIR_PIN 22  // != -1: Use pin for explicit DE/!RE

ESmart3 esmart3(rs485);  // Serial port to communicate with RS485 adapter


// Post data to InfluxDB
bool postInflux(const char *line) {
    static const char uri[] = "/write?db=" INFLUX_DB "&precision=s";

    http.begin(client, INFLUX_SERVER, INFLUX_PORT, uri);
    http.setUserAgent(PROGNAME);
    influx_status = http.POST(line);
    String payload;
    if (http.getSize() > 0) { // workaround for bug in getString()
        payload = http.getString();
    }
    http.end();

    if (influx_status < 200 || influx_status >= 300) {
        breathe_interval = err_interval;
        syslog.logf(LOG_ERR, "Post %s:%d%s status=%d line='%s' response='%s'",
            INFLUX_SERVER, INFLUX_PORT, uri, influx_status, line, payload.c_str());
        return false;
    }

    breathe_interval = ok_interval; // TODO mix with other possible errors
    post_time = time(NULL);         // TODO post_time?
    return true;
}


bool json_Information(char *json, size_t maxlen, ESmart3::Information_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"Information\":{"
        "\"Model\":\"%16.16s\","
        "\"Date\":\"%8.8s\","
        "\"FirmWare\":\"%4.4s\"}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)data.wSerial, 
        (char *)data.wModel, (char *)data.wDate, (char *)data.wFirmWare);

    return len < maxlen;
}


ESmart3::Information_t es3Information = {0};

// get device info once every minute
void handle_es3Information() {
    static const uint32_t interval = 60000;
    static uint32_t prev = 0 - interval + 0;  // check at start first

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::Information_t data = {0};
        if (esmart3.getInformation(data)) {
            if (strncmp((const char *)data.wSerialID, (const char *)es3Information.wSerialID, sizeof(data.wSerialID))) {
                // found a new/different eSmart3
                static const char lineFmt[] =
                    "Information,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "Model=\"%16.16s\","
                    "Date=\"%8.8s\","
                    "FirmWare=\"%4.4s\"";

                es3Information = data;
                json_Information(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)data.wSerial,
                    WiFi.getHostname(), (char *)data.wModel,
                    (char *)data.wDate, (char *)data.wFirmWare);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getInformation error");
        }
    }
}


bool json_ChgSts(char *json, size_t maxlen, ESmart3::ChgSts_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"ChgSts\":{"
        "\"ChgMode\":%u,"
        "\"PvVolt\":%u,"
        "\"BatVolt\":%u,"
        "\"ChgCurr\":%u,"
        "\"OutVolt\":%u,"
        "\"LoadVolt\":%u,"
        "\"LoadCurr\":%u,"
        "\"ChgPower\":%u,"
        "\"LoadPower\":%u,"
        "\"BatTemp\":%d,"
        "\"InnerTemp\":%d,"
        "\"BatCap\":%u,"
        "\"CO2\":%u,"
        "\"Fault\":\"%d%d%d%d%d%d%d%d%d%d\","
        "\"SystemReminder\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.wChgMode, data.wPvVolt, data.wBatVolt, data.wChgCurr, data.wOutVolt,
        data.wLoadVolt, data.wLoadCurr, data.wChgPower, data.wLoadPower, data.wBatTemp, 
        data.wInnerTemp, data.wBatCap, data.dwCO2, ESmart3::isBatteryVoltageOver(data.wFault), ESmart3::isPvVoltageOver(data.wFault),
        ESmart3::isChargeCurrentOver(data.wFault), ESmart3::isDischargeCurrentOver(data.wFault), ESmart3::isBatteryTemperatureAlarm(data.wFault),
        ESmart3::isInternalTemperatureAlarm(data.wFault), ESmart3::isPvVoltageLow(data.wFault), ESmart3::isBatteryVoltageLow(data.wFault),
        ESmart3::isTripZeroProtectionTrigger(data.wFault), ESmart3::isControlByManualSwitchgear(data.wFault), data.wSystemReminder);

    return len < maxlen;
}


ESmart3::ChgSts_t es3ChgSts = {0};

// get device status once every 1/2 second
void handle_es3ChgSts() {
    static const uint32_t interval = 500 + 50;
    static uint32_t prev = 0 - interval;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::ChgSts_t data = {0};
        if( esmart3.getChgSts(data) ) {
            if( memcmp(&data, &es3ChgSts, sizeof(data) ) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "ChgSts,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "ChgMode=%u,"
                    "PvVolt=%u,"
                    "BatVolt=%u,"
                    "ChgCurr=%u,"
                    "OutVolt=%u,"
                    "LoadVolt=%u,"
                    "LoadCurr=%u,"
                    "ChgPower=%u,"
                    "LoadPower=%u,"
                    "BatTemp=%d,"
                    "InnerTemp=%d,"
                    "BatCap=%u,"
                    "CO2=%u,"
                    "Fault=\"%d%d%d%d%d%d%d%d%d%d\","
                    "SystemReminder=%u";
                
                es3ChgSts = data;
                json_ChgSts(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.wChgMode, data.wPvVolt, data.wBatVolt, data.wChgCurr, data.wOutVolt,
                    data.wLoadVolt, data.wLoadCurr, data.wChgPower, data.wLoadPower, data.wBatTemp, 
                    data.wInnerTemp, data.wBatCap, data.dwCO2, ESmart3::isBatteryVoltageOver(data.wFault), ESmart3::isPvVoltageOver(data.wFault),
                    ESmart3::isChargeCurrentOver(data.wFault), ESmart3::isDischargeCurrentOver(data.wFault), ESmart3::isBatteryTemperatureAlarm(data.wFault),
                    ESmart3::isInternalTemperatureAlarm(data.wFault), ESmart3::isPvVoltageLow(data.wFault), ESmart3::isBatteryVoltageLow(data.wFault),
                    ESmart3::isTripZeroProtectionTrigger(data.wFault), ESmart3::isControlByManualSwitchgear(data.wFault), data.wSystemReminder);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getChgSts error");
        }
    }
}


bool json_BatParam(char *json, size_t maxlen, ESmart3::BatParam_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"BatParam\":{"
        "\"BatType\":%u,"
        "\"BatSysType\":%u,"
        "\"BulkVolt\":%u,"
        "\"FloatVolt\":%u,"
        "\"MaxChgCurr\":%u,"
        "\"MaxDisChgCurr\":%u,"
        "\"EqualizeChgVolt\":%u,"
        "\"EqualizeChgTime\":%u,"
        "\"LoadUseSel\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.wBatType, data.wBatSysType, data.wBulkVolt, data.wFloatVolt, data.wMaxChgCurr,
        data.wMaxDisChgCurr, data.wEqualizeChgVolt, data.wEqualizeChgTime, data.bLoadUseSel);

    return len < maxlen;
}


ESmart3::BatParam_t es3BatParam = {0};

// get battery parameters once every 10s
void handle_es3BatParam() {
    static const uint32_t interval = 10000;
    static uint32_t prev = 0 - interval + 100;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::BatParam_t data = {0};
        if( esmart3.getBatParam(data) ) {
            if( memcmp(&data, &es3BatParam, sizeof(data) ) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "BatParam,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "BatType=%u,"
                    "BatSysType=%u,"
                    "BulkVolt=%u,"
                    "FloatVolt=%u,"
                    "MaxChgCurr=%u,"
                    "MaxDisChgCurr=%u,"
                    "EqualizeChgVolt=%u,"
                    "EqualizeChgTime=%u,"
                    "LoadUseSel=%u";
                
                es3BatParam = data;
                json_BatParam(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.wBatType, data.wBatSysType, data.wBulkVolt, data.wFloatVolt, data.wMaxChgCurr,
                    data.wMaxDisChgCurr, data.wEqualizeChgVolt, data.wEqualizeChgTime, data.bLoadUseSel);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getBatParam error");
        }
    }
}


bool json_Log(char *json, size_t maxlen, ESmart3::Log_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"Log\":{"
        "\"RunTime\":%u,"
        "\"StartCnt\":%u,"
        "\"LastFaultInfo\":%u,"
        "\"FaultCnt\":%u,"
        "\"TodayEng\":%u,"
        "\"TodayEngDate\":\"%d:%d\","
        "\"MonthEng\":%u,"
        "\"MonthEngDate\":\"%d:%d\","
        "\"TotalEng\":%u,"
        "\"LoadTodayEng\":%u,"
        "\"LoadMonthEng\":%u,"
        "\"LoadTotalEng\":%u,"
        "\"BacklightTime\":%u,"
        "\"SwitchEnable\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.dwRunTime, data.wStartCnt, data.wLastFaultInfo, data.wFaultCnt, 
        data.dwTodayEng, data.wTodayEngDate.month, data.wTodayEngDate.day, data.dwMonthEng, 
        data.wMonthEngDate.month, data.wMonthEngDate.day, data.dwTotalEng, data.dwLoadTodayEng, 
        data.dwLoadMonthEng, data.dwLoadTotalEng, data.wBacklightTime, data.bSwitchEnable);

    return len < maxlen;
}


ESmart3::Log_t es3Log = {0};

// get status log once every 10s
void handle_es3Log() {
    static const uint32_t interval = 10000;
    static uint32_t prev = 0 - interval + 150;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::Log_t data = {0};
        if( esmart3.getLog(data) ) {
            if( memcmp(&data.wStartCnt, &es3Log.wStartCnt, sizeof(data) - offsetof(ESmart3::Log_t, wStartCnt) ) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "Log,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "RunTime=%u,"
                    "StartCnt=%u,"
                    "LastFaultInfo=%u,"
                    "FaultCnt=%u,"
                    "TodayEng=%u,"
                    "TodayEngDate=\"%d:%d\","
                    "MonthEng=%u,"
                    "MonthEngDate=\"%d:%d\","
                    "TotalEng=%u,"
                    "LoadTodayEng=%u,"
                    "LoadMonthEng=%u,"
                    "LoadTotalEng=%u,"
                    "BacklightTime=%u,"
                    "SwitchEnable=%u";
                
                es3Log = data;
                json_Log(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.dwRunTime, data.wStartCnt, data.wLastFaultInfo, data.wFaultCnt, 
                    data.dwTodayEng, data.wTodayEngDate.month, data.wTodayEngDate.day, data.dwMonthEng, 
                    data.wMonthEngDate.month, data.wMonthEngDate.day, data.dwTotalEng, data.dwLoadTodayEng, 
                    data.dwLoadMonthEng, data.dwLoadTotalEng, data.wBacklightTime, data.bSwitchEnable);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getLog error");
        }
    }
}


bool json_Parameters(char *json, size_t maxlen, ESmart3::Parameters_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"Parameters\":{"
        "\"PvVoltRatio\":%u,"
        "\"PvVoltOffset\":%u,"
        "\"BatVoltRatio\":%u,"
        "\"BatVoltOffset\":%u,"
        "\"ChgCurrRatio\":%u,"
        "\"ChgCurrOffset\":%u,"
        "\"LoadCurrRatio\":%u,"
        "\"LoadCurrOffset\":%u,"
        "\"LoadVoltRatio\":%u,"
        "\"LoadVoltOffset\":%u,"
        "\"OutVoltRatio\":%u,"
        "\"OutVoltOffset\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.wPvVoltRatio, data.wPvVoltOffset, data.wBatVoltRatio, data.wBatVoltOffset, 
        data.wChgCurrRatio, data.wChgCurrOffset, data.wLoadCurrRatio, data.wLoadCurrOffset, 
        data.wLoadVoltRatio, data.wLoadVoltOffset, data.wOutVoltRatio, data.wOutVoltOffset);

    return len < maxlen;
}


ESmart3::Parameters_t es3Parameters = {0};

// get calibration parameters once every 10s
void handle_es3Parameters() {
    static const uint32_t interval = 10000;
    static uint32_t prev = 0 - interval + 200;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::Parameters_t data = {0};
        if( esmart3.getParameters(data) ) {
            if( memcmp(&data, &es3Parameters, sizeof(data)) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "Parameters,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "PvVoltRatio=%u,"
                    "PvVoltOffset=%u,"
                    "BatVoltRatio=%u,"
                    "BatVoltOffset=%u,"
                    "ChgCurrRatio=%u,"
                    "ChgCurrOffset=%u,"
                    "LoadCurrRatio=%u,"
                    "LoadCurrOffset=%u,"
                    "LoadVoltRatio=%u,"
                    "LoadVoltOffset=%u,"
                    "OutVoltRatio=%u,"
                    "OutVoltOffset=%u";
                
                es3Parameters = data;
                json_Parameters(msg, sizeof(msg), data);
                // Serial.println(msg);
                // syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.wPvVoltRatio, data.wPvVoltOffset, data.wBatVoltRatio, data.wBatVoltOffset, 
                    data.wChgCurrRatio, data.wChgCurrOffset, data.wLoadCurrRatio, data.wLoadCurrOffset, 
                    data.wLoadVoltRatio, data.wLoadVoltOffset, data.wOutVoltRatio, data.wOutVoltOffset);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getParameters error");
        }
    }
}


bool json_LoadParam(char *json, size_t maxlen, ESmart3::LoadParam_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"LoadParam\":{"
        "\"LoadModuleSelect1\":%u,"
        "\"LoadModuleSelect2\":%u,"
        "\"LoadOnPvVolt\":%u,"
        "\"LoadOffPvVolt\":%u,"
        "\"PvContrlTurnOnDelay\":%u,"
        "\"PvContrlTurnOffDelay\":%u,"
        "\"AftLoadOnTime\":\"%d:%d\","
        "\"AftLoadOffTime\":\"%d:%d\","
        "\"MonLoadOnTime\":\"%d:%d\","
        "\"MonLoadOffTime\":\"%d:%d\","
        "\"LoadSts\":%u,"
        "\"Time2Enable\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.wLoadModuleSelect1, data.wLoadModuleSelect2, data.wLoadOnPvVolt, data.wLoadOffPvVolt, 
        data.wPvContrlTurnOnDelay, data.wPvContrlTurnOffDelay, data.AftLoadOnTime.hour, data.AftLoadOnTime.minute, 
        data.AftLoadOffTime.hour, data.AftLoadOffTime.minute, data.MonLoadOnTime.hour, data.MonLoadOnTime.minute, 
        data.MonLoadOffTime.hour, data.MonLoadOffTime.minute, data.wLoadSts, data.wTime2Enable);

    return len < maxlen;
}


ESmart3::LoadParam_t es3LoadParam = {0};

// get load parameters once every 10s
void handle_es3LoadParam() {
    static const uint32_t interval = 10000;
    static uint32_t prev = 0 - interval + 250;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::LoadParam_t data = {0};
        if( esmart3.getLoadParam(data) ) {
            if( memcmp(&data, &es3LoadParam, sizeof(data) ) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "LoadParam,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "LoadModuleSelect1=%u,"
                    "LoadModuleSelect2=%u,"
                    "LoadOnPvVolt=%u,"
                    "LoadOffPvVolt=%u,"
                    "PvContrlTurnOnDelay=%u,"
                    "PvContrlTurnOffDelay=%u,"
                    "AftLoadOnTime=\"%d:%d\","
                    "AftLoadOffTime=\"%d:%d\","
                    "MonLoadOnTime=\"%d:%d\","
                    "MonLoadOffTime=\"%d:%d\","
                    "LoadSts=%u,"
                    "Time2Enable=%u";
                
                es3LoadParam = data;
                json_LoadParam(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.wLoadModuleSelect1, data.wLoadModuleSelect2, data.wLoadOnPvVolt, data.wLoadOffPvVolt, 
                    data.wPvContrlTurnOnDelay, data.wPvContrlTurnOffDelay, data.AftLoadOnTime.hour, data.AftLoadOnTime.minute, 
                    data.AftLoadOffTime.hour, data.AftLoadOffTime.minute, data.MonLoadOnTime.hour, data.MonLoadOnTime.minute, 
                    data.MonLoadOffTime.hour, data.MonLoadOffTime.minute, data.wLoadSts, data.wTime2Enable);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getLoadParam error");
        }
    }
}


bool json_ProParam(char *json, size_t maxlen, ESmart3::ProParam_t data) {
    static const char jsonFmt[] =
        "{\"Version\":" VERSION ",\"Serial\":\"%8.8s\",\"ProParam\":{"
        "\"LoadOvp\":%u,"
        "\"LoadUvp\":%u,"
        "\"BatOvp\":%u,"
        "\"BatOvB\":%u,"
        "\"BatUvp\":%u,"
        "\"BatUvB\":%u}}";

    int len = snprintf(json, maxlen, jsonFmt, (char *)es3Information.wSerial,
        data.wLoadOvp, data.wLoadUvp, data.wBatOvp, data.wBatOvB, data.wBatUvp, data.wBatUvB);

    return len < maxlen;
}


ESmart3::ProParam_t es3ProParam = {0};

// get protection parameters once every 10s
void handle_es3ProParam() {
    static const uint32_t interval = 10000;
    static uint32_t prev = 0 - interval + 300;  // check at start + delay

    uint32_t now = millis();
    if( now - prev >= interval ) {
        prev += interval;
        ESmart3::ProParam_t data = {0};
        if( esmart3.getProParam(data) ) {
            if( memcmp(&data, &es3ProParam, sizeof(data) ) ) {
                // values have changed: publish
                static const char lineFmt[] =
                    "ProParam,Serial=%8.8s,Version=" VERSION " "
                    "Host=\"%s\","
                    "LoadOvp=%u,"
                    "LoadUvp=%u,"
                    "BatOvp=%u,"
                    "BatOvB=%u,"
                    "BatUvp=%u,"
                    "BatUvB=%u";
                
                es3ProParam = data;
                json_ProParam(msg, sizeof(msg), data);
                Serial.println(msg);
                syslog.log(LOG_INFO, msg);
                // TODO mqtt.publish(topic, msg);
                snprintf(msg, sizeof(msg), lineFmt, (char *)es3Information.wSerial, WiFi.getHostname(), 
                    data.wLoadOvp, data.wLoadUvp, data.wBatOvp, data.wBatOvB, data.wBatUvp, data.wBatUvB);
                postInflux(msg);
            }
        }
        else {
            Serial.println("getProParam error");
        }
    }
}


// Standard web page
const char *main_page( const char *body ) {
    static const char fmt[] =
        "<html>\n"
        " <head>\n"
        "  <title>" PROGNAME " %8.8s v" VERSION "</title>\n"
        "  <meta http-equiv=\"expires\" content=\"5\">\n"
        " </head>\n"
        " <body>\n"
        "  <h1>" PROGNAME " %8.8s v" VERSION "</h1>\n"
        "  <table><tr>\n"
        "   <td><form action=\"on\" method=\"post\">\n"
        "    <input type=\"submit\" name=\"on\" value=\"Load ON\" />\n"
        "   </form></td>\n"
        "   <td><form action=\"toggle\" method=\"post\">\n"
        "    <input type=\"submit\" name=\"toggle\" value=\"Toggle Load\" />\n"
        "   </form></td>\n"
        "   <td><form action=\"off\" method=\"post\">\n"
        "    <input type=\"submit\" name=\"off\" value=\"Load OFF\" />\n"
        "   </form></td>\n"
        "  </tr></table>\n%s<div>"
        "  <table>\n"
        "   <tr><td>Information</td><td><a href=\"/json/Information\">JSON</a></td></tr>\n"
        "   <tr><td>ChgSts</td><td><a href=\"/json/ChgSts\">JSON</a></td></tr>\n"
        "   <tr><td>BatParam</td><td><a href=\"/json/BatParam\">JSON</a></td></tr>\n"
        "   <tr><td>Log</td><td><a href=\"/json/Log\">JSON</a></td></tr>\n"
        "   <tr><td>Parameters</td><td><a href=\"/json/Parameters\">JSON</a></td></tr>\n"
        "   <tr><td>LoadParam</td><td><a href=\"/json/LoadParam\">JSON</a></td></tr>\n"
        "   <tr><td>ProParam</td><td><a href=\"/json/ProParam\">JSON</a></td></tr>\n"
        "   <tr><td>Post firmware image to</td><td><a href=\"/update\">/update</a></td></tr>\n"
        "   <tr><td>Last start time</td><td>%s</td></tr>\n"
        "   <tr><td>Last web update</td><td>%s</td></tr>\n"
        "   <tr><td>Last influx update</td><td>%s</td></tr>\n"
        "   <tr><td>Influx status</td><td>%d</td></tr>\n"
        "  </table>\n"
        "  <table><tr>\n"
        "   <td><form action=\"breathe\" method=\"post\">\n"
        "    <input type=\"submit\" name=\"breathe\" value=\"Breathe\" />\n"
        "   </form></td>\n"
        "   <td><form action=\"reset\" method=\"post\">\n"
        "    <input type=\"submit\" name=\"reset\" value=\"Reset\" />\n"
        "   </form></td>\n"
        "  </tr></table>\n"
        " </body>\n"
        "</html>\n";
    static char page[sizeof(fmt) + 500] = "";
    static char curr_time[30], influx_time[30];
    time_t now;
    time(&now);
    strftime(curr_time, sizeof(curr_time), "%FT%T%Z", localtime(&now));
    strftime(influx_time, sizeof(influx_time), "%FT%T%Z", localtime(&post_time));
    snprintf(page, sizeof(page), fmt, (char *)es3Information.wSerial,
        (char *)es3Information.wSerial, body, start_time, curr_time,
        influx_time, influx_status);
    return page;
}


// Define web pages for update, reset or for event infos
void setup_webserver() {
    web_server.on("/toggle", HTTP_POST, []() {
        bool on;
        const char *msg = "Load unknown";
        if (esmart3.getLoad(on)) {
            on = !on;
            if (esmart3.setLoad(on)) {
                msg = on ? "Load on" : "Load off";
            }
        }
        web_server.send(200, "text/html", main_page(msg)); 
    });

    web_server.on("/on", HTTP_POST, []() {
        bool on;
        const char *msg = "Load on";
        if (!esmart3.getLoad(on) || !on) {
            if (!esmart3.setLoad(true)) {
                msg = "Load unknown";
            }
        }
        web_server.send(200, "text/html", main_page(msg)); 
    });

    web_server.on("/off", HTTP_POST, []() {
        bool on;
        const char *msg = "Load off";
        if (!esmart3.getLoad(on) || on) {
            if (!esmart3.setLoad(false)) {
                msg = "Load unknown";
            }
        }
        web_server.send(200, "text/html", main_page(msg)); 
    });

    web_server.on("/json/Information", []() {
        json_Information(msg, sizeof(msg), es3Information);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/ChgSts", []() {
        json_ChgSts(msg, sizeof(msg), es3ChgSts);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/BatParam", []() {
        json_BatParam(msg, sizeof(msg), es3BatParam);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/Log", []() {
        json_Log(msg, sizeof(msg), es3Log);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/Parameters", []() {
        json_Parameters(msg, sizeof(msg), es3Parameters);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/LoadParam", []() {
        json_LoadParam(msg, sizeof(msg), es3LoadParam);
        web_server.send(200, "application/json", msg);
    });

    web_server.on("/json/ProParam", []() {
        json_ProParam(msg, sizeof(msg), es3ProParam);
        web_server.send(200, "application/json", msg);
    });

    // Call this page to reset the ESP
    web_server.on("/reset", HTTP_POST, []() {
        syslog.log(LOG_NOTICE, "RESET");
        web_server.send(200, "text/html",
                        "<html>\n"
                        " <head>\n"
                        "  <title>" PROGNAME " v" VERSION "</title>\n"
                        "  <meta http-equiv=\"refresh\" content=\"7; url=/\"> \n"
                        " </head>\n"
                        " <body>Resetting...</body>\n"
                        "</html>\n");
        delay(200);
        ESP.restart();
    });

    // Index page
    web_server.on("/", []() { 
        web_server.send(200, "text/html", main_page(""));
    });

    // Toggle breathing status led if you dont like it or ota does not work
    web_server.on("/breathe", HTTP_POST, []() {
        enabledBreathing = !enabledBreathing; 
        web_server.send(200, "text/html", main_page(enabledBreathing ? "breathing enabled" : "breathing disabled")); 
    });

    web_server.on("/breathe", HTTP_GET, []() {
        web_server.send(200, "text/html", main_page(enabledBreathing ? "breathing enabled" : "breathing disabled")); 
    });

    // Catch all page
    web_server.onNotFound( []() { 
        web_server.send(404, "text/html", main_page("<h2>page not found</h2>\n")); 
    });

    web_server.begin();

    MDNS.addService("http", "tcp", WEBSERVER_PORT);
    syslog.logf(LOG_NOTICE, "Serving HTTP on port %d", WEBSERVER_PORT);
}


// toggle load on key press
// pin is pulled up if released and pulled down if pressed
void handle_load_button( bool loadOn ) {
    static uint32_t prevTime = 0;
    static uint32_t debounceStatus = 1;
    static bool pressed = false;

    uint32_t now = millis();
    if( now - prevTime > 2 ) {  // debounce check every 2 ms, decision after 2ms/bit * 32bit = 64ms
        prevTime = now;

        // shift bits left, set lowest bit if button pressed
        debounceStatus = (debounceStatus << 1) | ((digitalRead(LOAD_BUTTON_PIN) == LOW) ? 1 : 0);

        if( debounceStatus == 0 && pressed ) {
            pressed = false;
        }
        else if( debounceStatus == 0xffffffff && !pressed ) {
            pressed = true;
            if (esmart3.setLoad(!loadOn)) {
                if( !loadOn ) {
                    Serial.println("Load switched ON");
                }
                else {
                    Serial.println("Load switched OFF");
                }
            }
            else {
                Serial.println("Load UNKNOWN");
            }
        }
        // else if (debounceStatus) {
        //     Serial.println(debounceStatus, HEX);
        // }
    }
}


// check once every 500ms if load status has changed
// return true if load is on (or unknown)
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
                    digitalWrite(LOAD_LED_PIN, LOAD_LED_ON);
                    Serial.println("Load is ON");
                }
                else {
                    digitalWrite(LOAD_LED_PIN, LOAD_LED_OFF);
                    Serial.println("Load is OFF");
                }
                prevStatus = true;
                prevLoad = loadOn;
            }
        }
        else {
            if( prevStatus ) {
                digitalWrite(LOAD_LED_PIN, LOAD_LED_ON);  // assume ON
                Serial.println("Load is UNKNOWN");
                prevStatus = false;
                prevLoad = true;
            }
        }
    }

    return prevLoad;
}


// check ntp status
// return true if time is valid
bool check_ntptime() {
    static bool have_time = false;

    #if defined(ESP32)
        bool valid_time = time(0) > 1582230020;
    #else
        ntp.update();
        bool valid_time = ntp.isTimeSet();
    #endif

    if (!have_time && valid_time) {
        have_time = true;
        time_t now = time(NULL);
        strftime(start_time, sizeof(start_time), "%FT%T%Z", localtime(&now));
        syslog.logf(LOG_NOTICE, "Got valid time at %s", start_time);
    }

    return have_time;
}


// Status led update
void handle_breathe() {
    static uint32_t start = 0;  // start of last breath
    static uint32_t min_duty = PWMRANGE / 20;  // limit min brightness
    static uint32_t max_duty = PWMRANGE / 2;  // limit max brightness
    static uint32_t prev_duty = 0;

    // map elapsed in breathing intervals
    uint32_t now = millis();
    uint32_t elapsed = now - start;
    if (elapsed > breathe_interval) {
        start = now;
        elapsed -= breathe_interval;
    }

    // map min brightness to max brightness twice in one breathe interval
    uint32_t duty = (max_duty - min_duty) * elapsed * 2 / breathe_interval + min_duty;
    if (duty > max_duty) {
        // second range: breathe out aka get darker
        duty = 2 * max_duty - duty;
    }

    duty = duty * duty / max_duty;  // generally reduce lower brightness levels

    if (duty != prev_duty) {
        // adjust pwm duty cycle
        prev_duty = duty;
        #if defined(ESP32)
            ledcWrite(HEALTH_PWM_CH, duty);
        #else
            analogWrite(HEALTH_LED_PIN, PWMRANGE - duty);
        #endif
    }
}


// Startup
void setup() {
    WiFi.mode(WIFI_STA);
    String host(HOSTNAME);
    host.toLowerCase();
    WiFi.hostname(host.c_str());

    pinMode(HEALTH_LED_PIN, OUTPUT);
    digitalWrite(HEALTH_LED_PIN, HEALTH_LED_ON);

    Serial.begin(BAUDRATE);
    Serial.println("\nStarting " PROGNAME " v" VERSION " " __DATE__ " " __TIME__);

    // Syslog setup
    syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
    syslog.deviceHostname(WiFi.getHostname());
    syslog.appName("Joba1");
    syslog.defaultPriority(LOG_KERN);

    digitalWrite(HEALTH_LED_PIN, HEALTH_LED_OFF);

    WiFiManager wm;
    // wm.resetSettings();
    if (!wm.autoConnect()) {
        Serial.println("Failed to connect WLAN");
        for (int i = 0; i < 1000; i += 200) {
            digitalWrite(HEALTH_LED_PIN, HEALTH_LED_ON);
            delay(100);
            digitalWrite(HEALTH_LED_PIN, HEALTH_LED_OFF);
            delay(100);
        }
        ESP.restart();
        while (true)
            ;
    }

    digitalWrite(HEALTH_LED_PIN, HEALTH_LED_ON);
    char msg[80];
    snprintf(msg, sizeof(msg), "%s Version %s, WLAN IP is %s", PROGNAME, VERSION,
        WiFi.localIP().toString().c_str());
    Serial.println(msg);
    syslog.logf(LOG_NOTICE, msg);

    #if defined(ESP8266)
        ntp.begin();
    #else
        configTime(0, 0, NTP_SERVER);
    #endif

    MDNS.begin(WiFi.getHostname());

    esp_updater.setup(&web_server);
    setup_webserver();

#if defined(ESP8266)
    rs485.begin(9600, SWSERIAL_8N1, 13, 15);  // Use pins 13 and 15 for RX and TX
    analogWriteRange(PWMRANGE);  // for health led breathing steps
#elif defined(ESP32)
    rs485.begin(9600, SERIAL_8N1);  // Use Serial2 default pins 16 and 17 for RX and TX
    ledcAttachPin(HEALTH_LED_PIN, HEALTH_PWM_CH);
    ledcSetup(HEALTH_PWM_CH, 1000, PWMBITS);
#else
    analogWriteRange(PWMRANGE);  // for health led breathing steps
    // init your non ESP serial port here
#endif

    pinMode(LOAD_BUTTON_PIN, INPUT_PULLUP);  // to toggle load status
    pinMode(LOAD_LED_PIN, OUTPUT);  // to show load status
    digitalWrite(LOAD_LED_PIN, LOAD_LED_OFF);

    esmart3.begin(RS485_DIR_PIN);

    Serial.println("Setup done");
}


// Main loop
void loop() {
    // TODO set/reset err_interval for breathing
    handle_es3Information();
    bool have_time = check_ntptime();
    if( es3Information.wSerial[0] ) {  // we have required esmart3 infos
        if (have_time && enabledBreathing) {
            handle_breathe();
        }
        handle_es3ChgSts();
        handle_es3BatParam();
        handle_es3Log();
        handle_es3Parameters();
        handle_es3ProParam();
        handle_es3LoadParam();
        // ignoring TempParam and EngSave (for now?)
    }
    handle_load_button(handle_load_led());
    web_server.handleClient();
}

/*
Keep array position of strings
matching the corresponding structure offsets
or enum values of *_t types in ESmart3.h
*/

#include <esmart3names.h>

#include <esmart3.h>


namespace ESMART3 {


// byte offsets

static const char *_header[] = { "start", "device", "address", "command", "item", "length" };

static const char *_dev[] { "ALL", "MPPT" };

static const char *_addr[] = { "BROADCAST" };

static const char *_cmd[] = { "ACK", "GET", "SET", "SET_NO_RESP", "NACK", "EXEC" /* no ERR */ };

static const char *_item[] = {
    "ChgSts",
    "BatParam",
    "Log",
    "Parameters",
    "LoadParam",
    "ChgDebug",
    "RemoteControl",
    "ProParam",
    "Information",
    "TempParam",
    "EngSave" };

static const char *_chgMode[] = { "CHG_WAIT", "CHG_MPPT", "CHG_BULK", "CHG_FLOAT", "CHG_PRE" };


// word offsets

static const char *_ChgSts[] = {
    "wChgMode",
    "wPvVolt",
    "wBatVolt",
    "wChgCurr",
    "wOutVolt",
    "wLoadVolt",
    "wLoadCurr",
    "wLoadColib",
    "wChgPower",
    "wLoadPower",
    "wBatTemp",
    "wInnerTemp",
    "wBatCap",
    "dwCO2",
    "",
    "wFault",
    "wSystemReminder" };

static const char *_BatParam[] = {
    "wFlag",
    "wBatType",
    "wBatSysType",
    "wBulkVolt",
    "wFloatVolt",
    "wMaxChgCurr",
    "wMaxDisChgCurr",
    "wEqualizeChgVolt",
    "wEqualizeChgTime",
    "bLoadUseSel",
    "ChkSum" };

static const char *_data[] = { "month", "day" };

static const char *_Log[] = {
    "wFlag",
    "dwRunTime",
    "",
    "wStartCnt",
    "wLastFaultInfo",
    "wFaultCnt",
    "dwTodayEng",
    "",
    "wTodayEngDate",
    "",
    "dwMonthEng",
    "",
    "wMonthEngDate",
    "",
    "dwTotalEng",
    "",
    "dwLoadTodayEng",
    "",
    "dwLoadMonthEng",
    "",
    "dwLoadTotalEng",
    "",
    "wBacklightTime",
    "bSwitchEnable",
    "ChkSum" };

static const char *_Parameters[] = {
    "wFlag",
    "wPvVoltRatio",
    "wPvVoltOffset",
    "wBatVoltRatio",
    "wBatVoltOffset",
    "wChgCurrRatio",
    "wChgCurrOffset",
    "wLoadCurrRatio",
    "wLoadCurrOffset",
    "wLoadVoltRatio",
    "wLoadVoltOffset",
    "wOutVoltRatio",
    "wOutVoltOffset",
    "ChkSum" };

static const char *_time[] = { "hour", "minute" };

static const char *_LoadParam[] = {
    "wFlag",
    "wLoadModuleSelect1",
    "wLoadModuleSelect2",
    "wLoadOnPvVolt",
    "wLoadOffPvVolt",
    "wPvContrlTurnOnDelay",
    "wPvContrlTurnOffDelay",
    "AftLoadOnTime",
    "",
    "AftLoadOffTime",
    "",
    "MonLoadOnTime",
    "",
    "MonLoadOffTime",
    "",
    "wLoadSts",
    "wTime2Enable",
    "ChkSum" };

static const char *_RemoteControl[] = {
    "uwMagicNum",
    "eRemoteCmd",
    "uwData" };

static const char *_ProParam[] = {
    "wFlag",
    "wLoadOvp",
    "wLoadUvp",
    "wBatOvp",
    "wBatOvB",
    "wBatUvp",
    "wBatUvB",
    "ChkSum" };

static const char *_Information[] = {
    "wFlag",
    "wSerialID",
    "",
    "",
    "",
    "wDate",
    "",
    "",
    "",
    "wFirmWare",
    "",
    "wModel",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ChkSum" };

static const char *_tempUnit[] = { "CELSIUS", "FAHRENHEIT" };

static const char *_TempParam[] = {
    "wFlag",
    "PeriodLoadCtr2Time",
    "",
    "bPVEnergySel",
    "bBatTempSel",
    "bLoadUseSel",
    "bLoadLaststate",
    "ChkSum" };

static const char *_EngSave[] = {
    "wFlag",
    "wMonthPower",      // 12 * uint32
    "wMonthLoadPower",  // 12 * uint32
    "wDayPower",        // 31 * uint32
    "wDayLoadPower",    // 31 * uint32
    "ChkSum" };

#define _OFFSET_TO_STRING(strings, offset) offset < sizeof(strings)/sizeof(*strings) ? strings[offset] : (""))
#define _OFFSET_TO_STRING(strings, offset) offset < sizeof(strings)/sizeof(*strings) ? strings[offset] : (""))

const char *header( size_t byte_offset ) { return _OFFSET_TO_STRING(_header, byte_offset); }
const char *dev( size_t byte_offset ) { return _OFFSET_TO_STRING(_dev, byte_offset); }
const char *addr( size_t byte_offset ) { return _OFFSET_TO_STRING(_addr, byte_offset); }
const char *cmd( size_t byte_offset ) { return _OFFSET_TO_STRING(_cmd, byte_offset); }
const char *item( size_t byte_offset ) { return _OFFSET_TO_STRING(_item, byte_offset); }
const char *chgMode( size_t byte_offset ) { return _OFFSET_TO_STRING(_chgMode, byte_offset); }
const char *ChgSts( size_t word_offset ) { return _OFFSET_TO_STRING(_ChgSts, word_offset); }
const char *BatParam( size_t word_offset ) { return _OFFSET_TO_STRING(_BatParam, word_offset); }
const char *data( size_t word_offset ) { return _OFFSET_TO_STRING(_data, word_offset); }
const char *Log( size_t word_offset ) { return _OFFSET_TO_STRING(_Log, word_offset); }
const char *Parameters( size_t word_offset ) { return _OFFSET_TO_STRING(_Parameters, word_offset); }
const char *time( size_t word_offset ) { return _OFFSET_TO_STRING(_time, word_offset); }
const char *LoadParam( size_t word_offset ) { return _OFFSET_TO_STRING(_LoadParam, word_offset); }
const char *RemoteControl( size_t word_offset ) { return _OFFSET_TO_STRING(_RemoteControl, word_offset); }
const char *ProParam( size_t word_offset ) { return _OFFSET_TO_STRING(_ProParam, word_offset); }
const char *Information( size_t word_offset ) { return _OFFSET_TO_STRING(_Information, word_offset); }
const char *tempUnit( size_t word_offset ) { return _OFFSET_TO_STRING(_tempUnit, word_offset); }
const char *TempParam( size_t word_offset ) { return _OFFSET_TO_STRING(_TempParam, word_offset); }

const char *EngSave( size_t word_offset ) {
    switch( word_offset ) {
        case offsetof(EngSave_t, wFlag) / 2:
        case offsetof(EngSave_t, wMonthPower) / 2:
            break;
        case offsetof(EngSave_t, wMonthLoadPower) / 2:
            word_offset = 2;
            break;
        case offsetof(EngSave_t, wDayPower) / 2:
            word_offset = 3;
            break;
        case offsetof(EngSave_t, wDayLoadPower) / 2:
            word_offset = 4;
            break;
        case offsetof(EngSave_t, ChkSum) / 2:
            word_offset = 5;
            break;
        default:
            word_offset = 6;
            break;
    }
    return _OFFSET_TO_STRING(_EngSave, word_offset); 
}

// Convert string to size_t struct offset or enum value
// return -1 for errors)
static size_t _to_str( const char *str, const char *strings[], size_t num_strings ) {
    if( !str || !*str ) {
        return (size_t)-1;
    }
    for( size_t i = 0; i < num_strings; i++ ) {
        if( strcmp(strings[i], str) == 0 ) {
            return i;
        }
    }
    return (size_t)-1;
}

#define _ARRAY_AND_SIZE(arr) arr, sizeof(arr)/sizeof(*arr)

size_t header_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_header)); }
size_t dev_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_dev)); }
size_t addr_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_addr)); }
size_t cmd_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_cmd)); }
size_t item_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_item)); }
size_t chgMode_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_chgMode)); }
size_t ChgSts_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_ChgSts)); }
size_t BatParam_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_BatParam)); }
size_t data_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_data)); }
size_t Log_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_Log)); }
size_t Parameters_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_Parameters)); }
size_t time_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_time)); }
size_t LoadParam_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_LoadParam)); }
size_t RemoteControl_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_RemoteControl)); }
size_t ProParam_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_ProParam)); }
size_t Information_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_Information)); }
size_t tempUnit_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_tempUnit)); }
size_t TempParam_str( const char *str ) { return _to_str(str, _ARRAY_AND_SIZE(_TempParam)); }

size_t EngSave_str( const char *str ) { 
    size_t pos = _to_str(str, _ARRAY_AND_SIZE(_EngSave);
    switch( pos ) {
        case 0:
        case 1:
            return pos;
        case 2:
            return offsetof(EngSave_t, wMonthLoadPower) / 2;
        case 3:
            return offsetof(EngSave_t, wDayPower) / 2;
        case 4:
            return offsetof(EngSave_t, wDayLoadPower) / 2;
        case 5:
            return offsetof(EngSave_t, ChkSum) / 2;
    }
    return (size_t)-1;
}

}

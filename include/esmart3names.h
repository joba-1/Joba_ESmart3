#ifndef ESMART3NAMES
#define ESMART3NAMES

/* 
Class to convert binary data of the ESmart3 RS485 protocol to strings

Author: Joachim.Banzhaf@gmail.com
License: GPL V2
*/

#include <stddef.h>

namespace ESMART3 {

const char *header( size_t byte_offset );
const char *dev( size_t byte_offset );
const char *addr( size_t byte_offset );
const char *cmd( size_t byte_offset );
const char *item( size_t byte_offset );
const char *chgMode( size_t byte_offset );
const char *ChgSts( size_t word_offset );
const char *BatParam( size_t word_offset );
const char *data( size_t word_offset );
const char *Log( size_t word_offset );
const char *Parameters( size_t word_offset );
const char *time( size_t word_offset );
const char *LoadParam( size_t word_offset );
const char *RemoteControl( size_t word_offset );
const char *ProParam( size_t word_offset );
const char *Information( size_t word_offset );
const char *tempUnit( size_t word_offset );
const char *TempParam( size_t word_offset );
const char *EngSave( size_t word_offset );

size_t header_str( const char *str );
size_t dev_str( const char *str );
size_t addr_str( const char *str );
size_t cmd_str( const char *str );
size_t item_str( const char *str );
size_t chgMode_str( const char *str );
size_t ChgSts_str( const char *str );
size_t BatParam_str( const char *str );
size_t data_str( const char *str );
size_t Log_str( const char *str );
size_t Parameters_str( const char *str );
size_t time_str( const char *str );
size_t LoadParam_str( const char *str );
size_t RemoteControl_str( const char *str );
size_t ProParam_str( const char *str );
size_t Information_str( const char *str );
size_t tempUnit_str( const char *str );
size_t TempParam_str( const char *str );
size_t EngSave_str( const char *str );

};

#endif

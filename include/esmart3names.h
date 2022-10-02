#ifndef ESMART3NAMES
#define ESMART3NAMES

/* 
Class to convert binary data of the ESmart3 RS485 protocol to strings

Author: Joachim.Banzhaf@gmail.com
License: GPL V2
*/

#include <stddef.h>

namespace ESMART3 {

const char *header_str( size_t byte_offset );
const char *dev_str(size_t byte_offset);
const char *addr_str(size_t byte_offset);
const char *cmd_str(size_t byte_offset);
const char *item_str(size_t byte_offset);
const char *chgMode_str(size_t byte_offset);
const char *ChgSts_str(size_t word_offset);
const char *BatParam_str(size_t word_offset);
const char *data_str(size_t word_offset);
const char *Log_str(size_t word_offset);
const char *Parameters_str(size_t word_offset);
const char *time_str(size_t word_offset);
const char *LoadParam_str(size_t word_offset);
const char *RemoteControl_str(size_t word_offset);
const char *ProParam_str(size_t word_offset);
const char *Information_str(size_t word_offset);
const char *tempUnit_str(size_t word_offset);
const char *TempParam_str(size_t word_offset);
const char *EngSave_str(size_t word_offset);

size_t header_offset( const char *str );
size_t dev_offset( const char *str );
size_t addr_offset( const char *str );
size_t cmd_offset( const char *str );
size_t item_offset( const char *str );
size_t chgMode_offset( const char *str );
size_t ChgSts_offset( const char *str );
size_t BatParam_offset( const char *str );
size_t data_offset( const char *str );
size_t Log_offset( const char *str );
size_t Parameters_offset( const char *str );
size_t time_offset( const char *str );
size_t LoadParam_offset( const char *str );
size_t RemoteControl_offset( const char *str );
size_t ProParam_offset( const char *str );
size_t Information_offset( const char *str );
size_t tempUnit_offset( const char *str );
size_t TempParam_offset( const char *str );
size_t EngSave_offset( const char *str );

};

#endif

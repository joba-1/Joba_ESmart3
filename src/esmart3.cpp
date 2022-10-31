#include <esmart3.h>

#include <string.h>


// Debug

static void hex( const char *label, const uint8_t *data, size_t length ) {
    Serial.printf("%s:", label);
    while( length-- ) {
        Serial.printf(" %02x", *(data++));
    }
    Serial.println();
}


// Basic methods

ESmart3::ESmart3( Stream &serial, uint8_t command_delay_ms ) 
    : _serial(serial), _delay(command_delay_ms), _prev(0), _dir_pin(-1) {
}

void ESmart3::begin( int dir_pin ) {
    _dir_pin = dir_pin;
    if( _dir_pin >= 0 ) {
        pinMode(_dir_pin, OUTPUT);
        digitalWrite(_dir_pin, LOW);  // read mode (default)
    }
}

bool ESmart3::execute( header_t &header, uint8_t *command, uint8_t *result ) {
    uint8_t crc;
    uint8_t offset[2];

    if( !prepareCmd(header, command, crc) ) {
        return false;
    }

    uint32_t remaining = _delay - (millis() - _prev);
    if( remaining <= _delay ) {
        delay(remaining);
    }

    if( _dir_pin >= 0 ) {
        digitalWrite(_dir_pin, HIGH);  // write mode
    }

    _serial.flush();  // make sure read buffer is empty
    bool rc = (_serial.write((uint8_t *)&header, sizeof(header)) == sizeof(header))
           && (_serial.write(command, header.length) == header.length)
           && (_serial.write(&crc, 1) == 1);
    _serial.flush();  // wait until write is done 

    if( _dir_pin >= 0 ) {
        digitalWrite(_dir_pin, LOW);  // read mode (default)
    }

    if( rc ) {
        rc = (_serial.readBytes((uint8_t *)&header, sizeof(header)) == sizeof(header))
          && (header.start == 0xaa && header.length <= 120)
          && (header.length < 2 || _serial.readBytes(offset, 2) == 2)
          && (header.length < 2 || (result && _serial.readBytes(result, header.length - 2) == header.length - 2))
          && (_serial.readBytes(&crc, 1) == 1)
          && isValid(header, header.length < 2 ? 0 : offset, result, crc);
    }

    _prev = millis();

    return rc;
}


// public Get-Commands

bool ESmart3::getChgSts( ChgSts_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, ChgSts, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getBatParam( BatParam_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, BatParam, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getLog( Log_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, Log, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getParameters( Parameters_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, Parameters, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getLoadParam( LoadParam_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, LoadParam, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getProParam( ProParam_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, ProParam, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getInformation( Information_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, Information, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getEngSave( EngSave_t &data, size_t start, size_t end ) {
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, EngSave, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, start, end);
    return execute(header, cmd, addr);
}

bool ESmart3::getLoad( bool &on ) {
    LoadParam_t data = {0};
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, LoadParam, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, 0x0f, 0x10);
    if( execute(header, cmd, addr) ) {
        on = (data.wLoadSts != 0);
        return true;
    }
    return false;
}

bool ESmart3::getDisplayTemperatureUnit( tempUnit_t &unit ) {
    TempParam_t data = {0};
    uint8_t cmd[3];
    header_t header = { 0, MPPT, BROADCAST, GET, TempParam, sizeof(cmd) };
    uint8_t *addr = initGetOffset(cmd, (uint8_t *)&data, 4, 5);
    if( execute(header, cmd, addr) ) {
        unit = data.bBatTempSel ? ESmart3::FAHRENHEIT : ESmart3::CELSIUS;
        return true;
    }
    return false;
}


// public Set-Commands

bool ESmart3::setBatParam( BatParam_t &data, size_t start, size_t end ) {
    uint8_t cmd[sizeof(data)];
    header_t header = { 0, MPPT, BROADCAST, SET, BatParam, sizeof(cmd) };
    uint16_t *words = (uint16_t *)&data;
    if( (end - start) * 2 >= sizeof(data) ) {
        return false;
    } 
    initSetOffset(cmd, (uint8_t *)&words[start], start, end);
    // hex("hdr", (uint8_t *)&header, sizeof(header));
    // hex("cmd", cmd, sizeof(cmd));
    return execute(header, cmd, 0) && header.command == ACK;
    // hex("hdr", (uint8_t *)&header, sizeof(header));
}

bool ESmart3::setProParam( ProParam_t &data, size_t start, size_t end ) {
    uint8_t cmd[sizeof(data)];
    header_t header = { 0, MPPT, BROADCAST, SET, ProParam, sizeof(cmd) };
    uint16_t *words = (uint16_t *)&data;
    if( (end - start) * 2 >= sizeof(data) ) {
        return false;
    } 
    initSetOffset(cmd, (uint8_t *)&words[start], start, end);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setMaxChargeCurrent( uint16_t deciAmps ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, BatParam, sizeof(cmd) };
    initSetOffset(cmd, (uint8_t *)&deciAmps, 0x05, 0x06);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setMaxLoadCurrent( uint16_t deciAmps ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, BatParam, sizeof(cmd) };
    initSetOffset(cmd, (uint8_t *)&deciAmps, 0x06, 0x07);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setBacklightTime( uint16_t sec ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, Log, sizeof(cmd) };
    initSetOffset(cmd, (uint8_t *)&sec, 0x16, 0x17);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setLoad( bool on ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, LoadParam, sizeof(cmd) };
    uint16_t loadStatus = on ? 5117 : 5118;
    initSetOffset(cmd, (uint8_t *)&loadStatus, 0x01, 0x02);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setTime( struct tm &now ) {
    uint8_t cmd[16];
    header_t header = { 0, MPPT, BROADCAST, SET, RemoteControl, sizeof(cmd) };
    int16_t timeBuf[9] = { 4, (int16_t)now.tm_year, (int16_t)now.tm_mon, (int16_t)now.tm_mday, (int16_t)now.tm_hour, (int16_t)now.tm_min, (int16_t)now.tm_sec };
    initSetOffset(cmd, (uint8_t *)timeBuf, 0x01, 0x08);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setDisplayTemperatureUnit( tempUnit_t unit ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, TempParam, sizeof(cmd) };
    initSetOffset(cmd, (uint8_t *)&unit, 0x04, 0x05);
    return execute(header, cmd, 0) && header.command == ACK;
}

bool ESmart3::setSwitchEnable( bool on ) {
    uint8_t cmd[4];
    header_t header = { 0, MPPT, BROADCAST, SET, Log, sizeof(cmd) };
    uint16_t switchEnable = on ? 1 : 0;
    initSetOffset(cmd, (uint8_t *)&switchEnable, 0x17, 0x18);
    // hex("hdr", (uint8_t *)&header, sizeof(header));
    // hex("cmd", cmd, sizeof(cmd));
    bool rc = execute(header, cmd, 0) && header.command == ACK;
    // hex("hdr", (uint8_t *)&header, sizeof(header));
    return rc;
}


// Private Stuff (used internally, not by library user)

// Prepare set commands that use offsets with range [start, end[
//   cmd: byte buffer with enough room for all data, i.e. (end - start) * 2 + 2
//   data: data to send (without offset)
void ESmart3::initSetOffset( uint8_t *cmd, uint8_t *data, size_t start, size_t end ) {
    cmd[0] = start;  // offset of data in item we are interested in (in 2-byte steps)
    cmd[1] = 0;
    memcpy(&cmd[2], data, (end - start) * 2);  // length of data in bytes
}

// Prepare get commands that use offsets with range [start, end[
//   cmd: 3-byte buffer for offset and length of the queried data
//   data: start of item structure that will receive the result
//   returns address in data where the queried data will begin
uint8_t *ESmart3::initGetOffset( uint8_t *cmd, uint8_t *data, size_t start, size_t end ) {
    cmd[0] = start;  // offset of data we are interested in (in 2-byte steps)
    cmd[1] = 0;
    cmd[2] = (end - start) * 2;  // length of data in bytes
    return data + cmd[0] * 2;  // structure offset in bytes
}

// Calculate 8-bit crc of command or result
// Return crc
uint8_t ESmart3::genCrc( header_t &header, uint8_t *offset, uint8_t *data ) {
    unsigned crc = 0;

    if( header.length > 120 ) {
        return 0xff;
    }

    size_t limit = header.length;
    if( limit >= 2 && offset ) {
        crc -= *(offset++);
        crc -= *offset;
        limit -= 2;
    }

    if( limit && !data ) {
        return 0xff;
    }

    while( limit-- ) {
        crc -= *(data++);
    }

    limit = sizeof(header);
    data = (uint8_t *)&header;
    while( limit-- ) {
        crc -= *(data++);
    }

    return crc & 0xff;
}

// Check crc of result
// Return true if calculated and stored crc match 
bool ESmart3::isValid( header_t &header, uint8_t *offset, uint8_t *data, uint8_t crc ) {
    if( header.start != 0xaa ) {
        return false;
    }
    return genCrc(header, offset, data) == crc;
}

// Set start and crc bytes of command
// Return length of command or 0 on errors
bool ESmart3::prepareCmd( header_t &header, uint8_t *command, uint8_t &crc ) {
    if( header.length > 120 || !command ) {
        return false;
    }
    header.start = 0xaa;
    crc = genCrc(header, 0, command);
    return true;
}

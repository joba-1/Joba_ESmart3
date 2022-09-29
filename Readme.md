# Joba_ESmart3

A library to communicate with an eSmart3 MPPT solar charge controller or similar device via RS485.

* See more on eSmart3 commands and wiring in include/esmart3.h
* See usage in examples/Test_ESmart3 project
* Init the serial port with 9600 baud and 8N1 before calling ESmart3 class methods.
 * ESP32 HardwareSerial, default pins
   ```
   Serial2.begin(9600, SERIAL_8N1);
   ```
 * ESP8266 SoftwareSerial
   ```
   SoftwareSerial port;
   ... 
   port.begin(9600, SWSERIAL_8N1, rx_pin, tx_pin);
   ```
 * ESP8266 hardware serial with alternate rx/tx pins 13/15 (not tested)
   ```
   Serial.begin(9600); Serial.swap();
   ```

Thanks @altelch and @skagmo for helpful information.

Comments welcome,
Joachim Banzhaf

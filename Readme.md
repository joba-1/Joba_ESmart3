# Joba_ESmart3

A library to communicate with an eSmart3 MPPT solar charge controller or similar device via RS485.

* See more on eSmart3 commands and wiring in include/esmart3.h
* See usage in examples/ directory
    * Test: uses most functions and prints results to check functionality
    * LiFePO: set parameters for charging LiFePO batteries. WARNING: I am no expert for LiFePO charging, better check before use :)
    * Monitor: regularly check most values of the device and report changes (on serial, syslog and influx db). Also provide values as json and allow toggling load output on a simple web interface. ![image](https://user-images.githubusercontent.com/32450554/197423230-299c6e02-67d4-47bb-9b30-9554a4f7b7b6.png) ![image](https://user-images.githubusercontent.com/32450554/197423734-46403d0d-ebc7-4324-af6e-e277b08397c2.png)


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

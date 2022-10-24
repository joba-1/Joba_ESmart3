# Joba_ESmart3

A library to communicate with an eSmart3 MPPT solar charge controller or similar device via RS485.

* See more on eSmart3 commands and wiring in include/esmart3.h
* See usage in examples/ directory
    * Test: uses most functions and prints results to check functionality
    * LiFePO: set parameters for charging LiFePO batteries. WARNING: I am no expert for LiFePO charging, better check before use :)
    * Monitor: regularly check most values of the device and report changes (on serial, syslog and influx db). 
      Also provide values as json and allow toggling load output on a simple web interface. 
      ![image](https://user-images.githubusercontent.com/32450554/197423230-299c6e02-67d4-47bb-9b30-9554a4f7b7b6.png) 
      ![image](https://user-images.githubusercontent.com/32450554/197423734-46403d0d-ebc7-4324-af6e-e277b08397c2.png)
      ```
      > influx -precision rfc3339 --database eSmart3 --execute "select * from ChgSts where time > '2022-10-24T07:32:26Z' and time < '2022-10-24T07:33:55Z' order by time"
      name: ChgSts
      time                 BatCap BatTemp BatVolt CO2     ChgCurr ChgMode ChgPower Fault      Host      InnerTemp LoadCurr LoadPower LoadVolt OutVolt PvVolt Serial   SystemReminder Version
      ----                 ------ ------- ------- ---     ------- ------- -------- -----      ----      --------- -------- --------- -------- ------- ------ ------   -------------- -------
      2022-10-24T07:32:27Z 96     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         0        0       1      34000083 0              1.0
      2022-10-24T07:33:41Z 96     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         132      0       1      34000083 0              1.0
      2022-10-24T07:33:42Z 96     22      130     1900544 0       0       0        0000001000 esmart3-1 23        157      202       129      0       1      34000083 0              1.0
      2022-10-24T07:33:43Z 95     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         0        0       1      34000083 0              1.0
      2022-10-24T07:33:44Z 96     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         0        0       1      34000083 0              1.0
      2022-10-24T07:33:48Z 96     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         132      0       1      34000083 0              1.0
      2022-10-24T07:33:50Z 95     22      126     1900544 0       0       0        0000001000 esmart3-1 23        423      520       123      0       1      34000083 0              1.0
      2022-10-24T07:33:51Z 95     22      126     1900544 0       0       0        0000001000 esmart3-1 23        433      532       123      0       1      34000083 0              1.0
      2022-10-24T07:33:52Z 95     22      126     1900544 0       0       0        0001001000 esmart3-1 23        424      521       123      0       1      34000083 0              1.0
      2022-10-24T07:33:53Z 95     22      126     1900544 0       0       0        0001001000 esmart3-1 23        421      517       123      0       1      34000083 0              1.0
      2022-10-24T07:33:54Z 95     22      132     1900544 0       0       0        0000001000 esmart3-1 23        0        0         0        0       1      34000083 0              1.0
      ```
* Init the serial port with 9600 baud and 8N1 before calling ESmart3 class methods.
    * ESP32 HardwareSerial, default pins
      ```c
      Serial2.begin(9600, SERIAL_8N1);
      ```
    * ESP8266 SoftwareSerial
      ```c
      SoftwareSerial port;
      ... 
      port.begin(9600, SWSERIAL_8N1, rx_pin, tx_pin);
      ```
    * ESP8266 hardware serial with alternate rx/tx pins 13/15 (not tested)
      ```c
      Serial.begin(9600); Serial.swap();
      ```
* Complete example
   * Toggle load
   ```c
   #include <Arduino.h>
   #include <esmart3.h>

   #define RS485_DIR_PIN 22              // Explicit DE/!RE pin, -1 if board does automatic direction

   ESmart3 esmart3(Serial2);             // Uses ESP32 2nd serial port to communicate with RS485 adapter

   void setup() {
      Serial2.begin(9600, SERIAL_8N1);  // Init serial port with default pins 16 and 17 for RX and TX
      esmart3.begin(RS485_DIR_PIN);     // Init RS485 communication
   }

   void loop() {
      bool on_off;
      
      if (esmart3.getLoad(on_off)) {    // Get current load output status
         esmart3.setLoad(!on_off);     // Toggle current load output
      }

      delay(10000);
   }
   ```
   
Thanks @altelch and @skagmo for helpful information.

Comments welcome,
Joachim Banzhaf

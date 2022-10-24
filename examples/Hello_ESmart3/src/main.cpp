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

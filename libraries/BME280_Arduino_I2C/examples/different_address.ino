#include <Arduino.h>
#include <BME280_Arduino_I2C.h>

BME280_Arduino_I2C bme(0x77);

void setup() {
    Serial.begin(9600);

    // Initialize BME280_Arduino_I2C library
    if (bme.begin() == 0) {
        Serial.println("BME280 initialized");
    } else {
        /*
            Returning code 1: Wire is not available
            Returning code 2: Device has not been found
        */
        Serial.println("BME280 failed to initialize");
    }
}

void loop() {
    // Read measurements from the sensor
    BME280Data* data = bme.read();

    // Check if data is received. If data could not be received, data would be a null pointer
    if (data != nullptr) {
        Serial.print("> Temperature (C): ");
        Serial.println(data->temperature);
        Serial.print("> Humidity (%): ");
        Serial.println(data->humidity);
        Serial.print("> Pressure (Pa): ");
        Serial.println(data->pressure);
    }

    delay(1000);
}
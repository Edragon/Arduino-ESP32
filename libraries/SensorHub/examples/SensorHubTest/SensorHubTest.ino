#include <SensorHub.h>

SensorHub sensor(0x40); // Example I2C address, replace it with the actual address of your sensor

void setup() {
    Serial.begin(115200);
    if (sensor.is_sensor_connected()) {
        Serial.println("Sensor detected!");
    } else {
        Serial.println("Sensor not detected.");
    }
}

void loop() {
    Serial.println(sensor.i2c_readByte(0x00, 1));
    delay(1000);
}

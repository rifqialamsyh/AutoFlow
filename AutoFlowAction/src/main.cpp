#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(4, 5); // CE, CSN pins

struct SensorData {
  float temperature;
  int soilMoistureValue;
};

struct UltrasonicData {
  float distance;
};

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1, 0xF0F0F0F0E1LL); // Set the address for Transmitter 1
  radio.openReadingPipe(2, 0xF0F0F0F0E2LL); // Set the address for Transmitter 2
  radio.setPALevel(RF24_PA_LOW);           // Set power level
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    // Determine which pipe has data available
    uint8_t pipeNum;
    while (radio.available(&pipeNum)) {
      // Check if the data is from Transmitter 1
      if (pipeNum == 1) {
        // Receive the bundle from Transmitter 1
        SensorData receivedSensorData;
        radio.read(&receivedSensorData, sizeof(receivedSensorData));

        // Print received sensor data
        Serial.print("Temperature (Transmitter 1): ");
        Serial.print(receivedSensorData.temperature);
        Serial.print(" °C, Soil Moisture (Transmitter 1): ");
        Serial.println(receivedSensorData.soilMoistureValue);
      }
      // Check if the data is from Transmitter 2
      else if (pipeNum == 2) {
        // Receive the bundle from Transmitter 2
        UltrasonicData receivedUltrasonicData;
        radio.read(&receivedUltrasonicData, sizeof(receivedUltrasonicData));

        // Print received ultrasonic data
        Serial.print("Ultrasonic Distance (Transmitter 2): ");
        Serial.print(receivedUltrasonicData.distance);
        Serial.println(" cm");
      }
    }
  }
}
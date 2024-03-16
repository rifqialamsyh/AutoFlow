#include <DHT.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define DHTPIN 3 // Pin connected to DHT11 sensor
#define DHTTYPE DHT11 // DHT11 sensor type

DHT dht(DHTPIN, DHTTYPE);

const int soilMoisturePin = A6; // Pin connected to soil moisture sensor

RF24 radio(7, 8); // CE, CSN pins

struct SensorData {
  float temperature;
  int soilMoistureValue;
};

void setup() {
  Serial.begin(9600);
  dht.begin();
  radio.begin();
  radio.openWritingPipe(0xF0F0F0F0E1LL); // Set the address for communication
  radio.setPALevel(RF24_PA_LOW);        // Set power level
}

void loop() {
  // Read temperature from DHT11 sensor
  float temperature = dht.readTemperature();

  // Read soil moisture value
  int soilMoistureValue = analogRead(soilMoisturePin);

  // Bundle the sensor data
  SensorData data;
  data.temperature = temperature;
  data.soilMoistureValue = soilMoistureValue;

  // Send the data via NRF24L01
  radio.write(&data, sizeof(data));
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Soil Moisture: ");
  Serial.println(soilMoistureValue);
  // Wait before sending again
  delay(1000); // Delay for 2 seconds
}

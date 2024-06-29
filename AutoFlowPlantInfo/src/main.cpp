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
  int soilMoisturePercentage; // Change to percentage
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
  // Adjust the temperature value for tuning
  temperature -= 2.0;

  // Read soil moisture value
  int soilMoistureValue = analogRead(soilMoisturePin);
  
  // Map the soil moisture value to a percentage scale
  int soilMoisturePercentage = map(soilMoistureValue, 0, 1023, 100, 0);

  // Bundle the sensor data
  SensorData data;
  data.temperature = temperature;
  data.soilMoisturePercentage = soilMoisturePercentage; // Store percentage instead of raw value

  // Send the data via NRF24L01
  radio.write(&data, sizeof(data));
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Soil Moisture: ");
  Serial.print(soilMoisturePercentage); // Print percentage instead of raw value
  Serial.println("%");

  // Wait before sending again
  delay(1000); // Delay for 1 second
}

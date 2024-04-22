#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

RF24 radio(4, 5); // CE, CSN pins

const char* ssid = "ssid ";
const char* password = "password ";
const char* apiEndpoint = "apiEndpoint ";

struct SensorData {
  float temperature;
  int soilMoisturePercentage;
};

struct UltrasonicData {
  float depth;
};

SensorData latestSensorData;
UltrasonicData latestUltrasonicData;

unsigned long lastTransmissionTime = 0;
const unsigned long transmissionInterval = 1 * 60 * 1000; // 1 minute in milliseconds

void sendDataToAPI();
void sendPOSTRequest(String jsonData);

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1, 0xF0F0F0F0E1LL); // Set the address for Transmitter 1
  radio.openReadingPipe(2, 0xF0F0F0F0E2LL); // Set the address for Transmitter 2
  radio.setPALevel(RF24_PA_LOW);           // Set power level
  radio.startListening();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi");

  // Set the last transmission time to the current time
  lastTransmissionTime = millis();
}

void loop() {
  // Check if it's time to send data
  if (millis() - lastTransmissionTime >= transmissionInterval) {
    // Send data to API
    sendDataToAPI();

    // Update the last transmission time
    lastTransmissionTime = millis();
  }

  // Check for data from NRF24L01
  if (radio.available()) {
    // Determine which pipe has data available
    uint8_t pipeNum;
    while (radio.available(&pipeNum)) {
      // Check if the data is from Transmitter 1
      if (pipeNum == 1) {
        // Receive the bundle from Transmitter 1
        radio.read(&latestSensorData, sizeof(latestSensorData));
        Serial.print("Received Sensor Data - Temperature: ");
        Serial.print(latestSensorData.temperature);
        Serial.print(" °C, Soil Moisture: ");
        Serial.println(latestSensorData.soilMoisturePercentage);
      }
      // Check if the data is from Transmitter 2
      else if (pipeNum == 2) {
        // Receive the bundle from Transmitter 2
        radio.read(&latestUltrasonicData, sizeof(latestUltrasonicData));
        Serial.print("Received Ultrasonic Data - Depth: ");
        Serial.println(latestUltrasonicData.depth);
      }
    }
  }

  // Other tasks can be performed here
}

void sendDataToAPI() {
  // Create JSON object
  StaticJsonDocument<200> doc;
  // doc["temperature"] = latestSensorData.temperature;
  // doc["soil_moisture"] = latestSensorData.soilMoisturePercentage;
  // doc["amount_of_water"] = latestUltrasonicData.depth;

  char tempString[6]; // Allocate enough space for the string representation
  sprintf(tempString, "%.1f", latestSensorData.temperature); // Convert temperature to string
  doc["temperature"] = tempString;
  doc["soil_moisture"] = latestSensorData.soilMoisturePercentage;
  // Format amount of water to one decimal place
  char waterString[6]; // Allocate enough space for the string representation
  sprintf(waterString, "%.1f", latestUltrasonicData.depth); // Convert amount of water to string
  doc["amount_of_water"] = waterString;

  // Serialize JSON to string
  String jsonData;
  serializeJson(doc, jsonData);
  Serial.println("JSON Data: " + jsonData);

  // Send POST request to API
  sendPOSTRequest(jsonData);
}

void sendPOSTRequest(String jsonData) {
  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

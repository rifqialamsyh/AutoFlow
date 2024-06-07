#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Fuzzy.h>

#define RELAY_PIN 16

RF24 radio(4, 5); // CE, CSN pins

const char* ssid = "ssid";
const char* password = "wifi-pass";
const char* apiEndpoint = "api-endpoint";

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

Adafruit_SSD1306 display(128, 64, &Wire, -1);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
Fuzzy *fuzzyWatering = new Fuzzy();

void sendDataToAPI();
void sendPOSTRequest(String jsonData);
void displayOnOled();
void initializeFuzzyController();

int wateringDuration;  // Duration in seconds

// Fuzzy Sets for Soil Moisture
FuzzySet *dry = new FuzzySet(0, 0, 25, 50);
FuzzySet *moist = new FuzzySet(25, 50, 50, 75);
FuzzySet *wet = new FuzzySet(50, 75, 100, 100);

// Fuzzy Sets for Temperature
FuzzySet *lowTemp = new FuzzySet(0, 0, 15, 25);
FuzzySet *mediumTemp = new FuzzySet(15, 25, 25, 35);
FuzzySet *highTemp = new FuzzySet(25, 35, 50, 50);

// Fuzzy Sets for Water Amount
FuzzySet *lowWater = new FuzzySet(0, 0, 25, 50);
FuzzySet *mediumWater = new FuzzySet(25, 50, 50, 75);
FuzzySet *highWater = new FuzzySet(50, 75, 100, 100);

// Fuzzy Sets for Watering Duration
FuzzySet *shortDuration = new FuzzySet(0, 0, 5, 10);
FuzzySet *mediumDuration = new FuzzySet(5, 10, 10, 15);
FuzzySet *longDuration = new FuzzySet(10, 15, 20, 20);

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1, 0xF0F0F0F0E1LL);
  radio.openReadingPipe(2, 0xF0F0F0F0E2LL);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to WiFi");

  lastTransmissionTime = millis();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  timeClient.begin();
  timeClient.setTimeOffset(25200); // Adjust according to your timezone

  initializeFuzzyController();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  displayOnOled();

  unsigned long currentMillis = millis();
  if (currentMillis - lastTransmissionTime >= transmissionInterval) {
    sendDataToAPI();
    lastTransmissionTime = currentMillis;
  }

  if (radio.available()) {
    uint8_t pipeNum;
    while (radio.available(&pipeNum)) {
      if (pipeNum == 1) {
        radio.read(&latestSensorData, sizeof(latestSensorData));
      } else if (pipeNum == 2) {
        radio.read(&latestUltrasonicData, sizeof(latestUltrasonicData));
      }
    }
  }

  timeClient.update();
  setTime(timeClient.getEpochTime());
  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  // Check if it is one of the specified times and the minutes and seconds are zero
  // OR check if it's exactly 21:30
  if (((currentHour == 9 || currentHour == 12 || currentHour == 15) && currentMinute == 0 && currentSecond == 0) ||
      (currentHour == 22 && currentMinute == 15 && currentSecond == 0) ||
      (currentHour == 22 && currentMinute == 18 && currentSecond == 0) ||
      (currentHour == 22 && currentMinute == 21 && currentSecond == 0)) {
    fuzzyWatering->setInput(1, latestSensorData.soilMoisturePercentage);
    fuzzyWatering->setInput(2, latestSensorData.temperature);
    fuzzyWatering->setInput(3, latestUltrasonicData.depth);

    fuzzyWatering->fuzzify();
    int wateringDuration = fuzzyWatering->defuzzify(1);

    Serial.println("Watering Duration: " + String(wateringDuration) + " seconds");

    digitalWrite(RELAY_PIN, HIGH);
    delay(wateringDuration * 1000);
    digitalWrite(RELAY_PIN, LOW);
  }
}


void sendDataToAPI() {
  // Create JSON object
  StaticJsonDocument<200> doc;

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

void displayOnOled() {
    // Update time
  timeClient.update();

  // Clear the display
  display.clearDisplay();

  // Print Jakarta time
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(timeClient.getFormattedTime());

  // Print WiFi status
  display.setTextSize(1);
  display.setCursor(53, 0);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("Connected");
  } else {
    display.print("Disconnected");
  }

  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print("Temperature: ");
  display.print(latestSensorData.temperature);

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("Soil Moisture: ");
  display.print(latestSensorData.soilMoisturePercentage);

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Water in Liters: ");
  display.print(latestUltrasonicData.depth);

  // Display updates
  display.display();

  // Delay for a second
  delay(1000);
}

void initializeFuzzyController() {
  FuzzyInput *soilMoistureInput = new FuzzyInput(1);
  soilMoistureInput->addFuzzySet(dry);
  soilMoistureInput->addFuzzySet(moist);
  soilMoistureInput->addFuzzySet(wet);
  fuzzyWatering->addFuzzyInput(soilMoistureInput);

  // Fuzzy Input for Temperature
  FuzzyInput *temperatureInput = new FuzzyInput(2);
  temperatureInput->addFuzzySet(lowTemp);
  temperatureInput->addFuzzySet(mediumTemp);
  temperatureInput->addFuzzySet(highTemp);
  fuzzyWatering->addFuzzyInput(temperatureInput);

  // Fuzzy Input for Water Amount
  FuzzyInput *waterAmountInput = new FuzzyInput(3);
  waterAmountInput->addFuzzySet(lowWater);
  waterAmountInput->addFuzzySet(mediumWater);
  waterAmountInput->addFuzzySet(highWater);
  fuzzyWatering->addFuzzyInput(waterAmountInput);

  // Fuzzy Output for Watering Duration
  FuzzyOutput *wateringDurationOutput = new FuzzyOutput(1);
  wateringDurationOutput->addFuzzySet(shortDuration);
  wateringDurationOutput->addFuzzySet(mediumDuration);
  wateringDurationOutput->addFuzzySet(longDuration);
  fuzzyWatering->addFuzzyOutput(wateringDurationOutput);

  // Define Fuzzy Rules
  // Dry soil moisture rules
  FuzzyRuleAntecedent *dryLowTempLowWater = new FuzzyRuleAntecedent();
  dryLowTempLowWater->joinWithAND(dry, lowTemp);
  FuzzyRuleAntecedent *fordryLowTempLowWater = new FuzzyRuleAntecedent();
  fordryLowTempLowWater->joinWithAND(dryLowTempLowWater, lowWater);

  FuzzyRuleAntecedent *dryLowTempMediumWater = new FuzzyRuleAntecedent();
  dryLowTempMediumWater->joinWithAND(dry, lowTemp);
  FuzzyRuleAntecedent *fordryLowTempMediumWater = new FuzzyRuleAntecedent();
  fordryLowTempMediumWater->joinWithAND(dryLowTempMediumWater, mediumWater);
  
  FuzzyRuleAntecedent *dryLowTempHighWater = new FuzzyRuleAntecedent();
  dryLowTempHighWater->joinWithAND(dry, lowTemp);
  FuzzyRuleAntecedent *fordryLowTempHighWater = new FuzzyRuleAntecedent();
  fordryLowTempHighWater->joinWithAND(dryLowTempHighWater, highWater);

  FuzzyRuleAntecedent *dryMediumTempLowWater = new FuzzyRuleAntecedent();
  dryMediumTempLowWater->joinWithAND(dry, mediumTemp);
  FuzzyRuleAntecedent *fordryMediumTempLowWater = new FuzzyRuleAntecedent();
  fordryMediumTempLowWater->joinWithAND(dryMediumTempLowWater, lowWater);

  FuzzyRuleAntecedent *dryMediumTempMediumWater = new FuzzyRuleAntecedent();
  dryMediumTempMediumWater->joinWithAND(dry, mediumTemp);
  FuzzyRuleAntecedent *fordryMediumTempMediumWater = new FuzzyRuleAntecedent();
  fordryMediumTempMediumWater->joinWithAND(dryMediumTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *dryMediumTempHighWater = new FuzzyRuleAntecedent();
  dryMediumTempHighWater->joinWithAND(dry, mediumTemp);
  FuzzyRuleAntecedent *fordryMediumTempHighWater = new FuzzyRuleAntecedent();
  fordryMediumTempHighWater->joinWithAND(dryMediumTempHighWater, highWater);

  FuzzyRuleAntecedent *dryHighTempLowWater = new FuzzyRuleAntecedent();
  dryHighTempLowWater->joinWithAND(dry, highTemp);
  FuzzyRuleAntecedent *fordryHighTempLowWater = new FuzzyRuleAntecedent();
  fordryHighTempLowWater->joinWithAND(dryHighTempLowWater, lowWater);

  FuzzyRuleAntecedent *dryHighTempMediumWater = new FuzzyRuleAntecedent();
  dryHighTempMediumWater->joinWithAND(dry, highTemp);
  FuzzyRuleAntecedent *fordryHighTempMediumWater = new FuzzyRuleAntecedent();
  fordryHighTempMediumWater->joinWithAND(dryHighTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *dryHighTempHighWater = new FuzzyRuleAntecedent();
  dryHighTempHighWater->joinWithAND(dry, highTemp);
  FuzzyRuleAntecedent *fordryHighTempHighWater = new FuzzyRuleAntecedent();
  fordryHighTempHighWater->joinWithAND(dryHighTempHighWater, highWater);

  FuzzyRuleConsequent *thenLongDuration = new FuzzyRuleConsequent();
  FuzzyRuleConsequent *thenMediumDuration = new FuzzyRuleConsequent();
  FuzzyRuleConsequent *thenShortDuration = new FuzzyRuleConsequent();

  thenLongDuration->addOutput(longDuration);
  thenMediumDuration->addOutput(mediumDuration);
  thenShortDuration->addOutput(shortDuration);

  fuzzyWatering->addFuzzyRule(new FuzzyRule(1, fordryLowTempLowWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(2, fordryLowTempMediumWater, thenLongDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(3, fordryLowTempHighWater, thenLongDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(4, fordryMediumTempLowWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(5, fordryMediumTempMediumWater, thenLongDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(6, fordryMediumTempHighWater, thenLongDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(7, fordryHighTempLowWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(8, fordryHighTempMediumWater, thenLongDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(9, fordryHighTempHighWater, thenLongDuration));

  // Moist soil moisture rules
  FuzzyRuleAntecedent *moistLowTempLowWater = new FuzzyRuleAntecedent();
  moistLowTempLowWater->joinWithAND(moist, lowTemp);
  FuzzyRuleAntecedent *formoistLowTempLowWater = new FuzzyRuleAntecedent();
  formoistLowTempLowWater->joinWithAND(moistLowTempLowWater, lowWater);

  FuzzyRuleAntecedent *moistLowTempMediumWater = new FuzzyRuleAntecedent();
  moistLowTempMediumWater->joinWithAND(moist, lowTemp);
  FuzzyRuleAntecedent *formoistLowTempMediumWater = new FuzzyRuleAntecedent();
  formoistLowTempMediumWater->joinWithAND(moistLowTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *moistLowTempHighWater = new FuzzyRuleAntecedent();
  moistLowTempHighWater->joinWithAND(moist, lowTemp);
  FuzzyRuleAntecedent *formoistLowTempHighWater = new FuzzyRuleAntecedent();
  formoistLowTempHighWater->joinWithAND(moistLowTempHighWater, highWater);


  FuzzyRuleAntecedent *moistMediumTempLowWater = new FuzzyRuleAntecedent();
  moistMediumTempLowWater->joinWithAND(moist, mediumTemp);
  FuzzyRuleAntecedent *formoistMediumTempLowWater = new FuzzyRuleAntecedent();
  formoistMediumTempLowWater->joinWithAND(moistMediumTempLowWater, lowWater);

  FuzzyRuleAntecedent *moistMediumTempMediumWater = new FuzzyRuleAntecedent();
  moistMediumTempMediumWater->joinWithAND(moist, mediumTemp);
  FuzzyRuleAntecedent *formoistMediumTempMediumWater = new FuzzyRuleAntecedent();
  formoistMediumTempMediumWater->joinWithAND(moistMediumTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *moistMediumTempHighWater = new FuzzyRuleAntecedent();
  moistMediumTempHighWater->joinWithAND(moist, mediumTemp);
  FuzzyRuleAntecedent *formoistMediumTempHighWater = new FuzzyRuleAntecedent();
  formoistMediumTempHighWater->joinWithAND(moistMediumTempHighWater, highWater);

  FuzzyRuleAntecedent *moistHighTempLowWater = new FuzzyRuleAntecedent();
  moistHighTempLowWater->joinWithAND(moist, highTemp);
  FuzzyRuleAntecedent *formoistHighTempLowWater = new FuzzyRuleAntecedent();
  formoistHighTempLowWater->joinWithAND(moistHighTempLowWater, lowWater);

  FuzzyRuleAntecedent *moistHighTempMediumWater = new FuzzyRuleAntecedent();
  moistHighTempMediumWater->joinWithAND(moist, highTemp);
  FuzzyRuleAntecedent *formoistHighTempMediumWater = new FuzzyRuleAntecedent();
  formoistHighTempMediumWater->joinWithAND(moistHighTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *moistHighTempHighWater = new FuzzyRuleAntecedent();
  moistHighTempHighWater->joinWithAND(moist, highTemp);
  FuzzyRuleAntecedent *formoistHighTempHighWater = new FuzzyRuleAntecedent();
  formoistHighTempHighWater->joinWithAND(moistHighTempHighWater, highWater);

  fuzzyWatering->addFuzzyRule(new FuzzyRule(10, formoistLowTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(11, formoistLowTempMediumWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(12, formoistLowTempHighWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(13, formoistMediumTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(14, formoistMediumTempMediumWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(15, formoistMediumTempHighWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(16, formoistHighTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(17, formoistHighTempMediumWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(18, formoistHighTempHighWater, thenMediumDuration));

  // Wet soil moisture rules
  FuzzyRuleAntecedent *wetLowTempLowWater = new FuzzyRuleAntecedent();
  wetLowTempLowWater->joinWithAND(wet, lowTemp);
  FuzzyRuleAntecedent *forwetLowTempLowWater = new FuzzyRuleAntecedent();
  forwetLowTempLowWater->joinWithAND(wetLowTempLowWater, lowWater);

  FuzzyRuleAntecedent *wetLowTempMediumWater = new FuzzyRuleAntecedent();
  wetLowTempMediumWater->joinWithAND(wet, lowTemp);
  FuzzyRuleAntecedent *forwetLowTempMediumWater = new FuzzyRuleAntecedent();
  forwetLowTempMediumWater->joinWithAND(wetLowTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *wetLowTempHighWater = new FuzzyRuleAntecedent();
  wetLowTempHighWater->joinWithAND(wet, lowTemp);
  FuzzyRuleAntecedent *forwetLowTempHighWater = new FuzzyRuleAntecedent();
  forwetLowTempHighWater->joinWithAND(wetLowTempHighWater, highWater);

  FuzzyRuleAntecedent *wetMediumTempLowWater = new FuzzyRuleAntecedent();
  wetMediumTempLowWater->joinWithAND(wet, mediumTemp);
  FuzzyRuleAntecedent *forwetMediumTempLowWater = new FuzzyRuleAntecedent();
  forwetMediumTempLowWater->joinWithAND(wetMediumTempLowWater, lowWater);

  FuzzyRuleAntecedent *wetMediumTempMediumWater = new FuzzyRuleAntecedent();
  wetMediumTempMediumWater->joinWithAND(wet, mediumTemp);
  FuzzyRuleAntecedent *forwetMediumTempMediumWater = new FuzzyRuleAntecedent();
  forwetMediumTempMediumWater->joinWithAND(wetMediumTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *wetMediumTempHighWater = new FuzzyRuleAntecedent();
  wetMediumTempHighWater->joinWithAND(wet, mediumTemp);
  FuzzyRuleAntecedent *forwetMediumTempHighWater = new FuzzyRuleAntecedent();
  forwetMediumTempHighWater->joinWithAND(wetMediumTempHighWater, highWater);

  FuzzyRuleAntecedent *wetHighTempLowWater = new FuzzyRuleAntecedent();
  wetHighTempLowWater->joinWithAND(wet, highTemp);
  FuzzyRuleAntecedent *forwetHighTempLowWater = new FuzzyRuleAntecedent();
  forwetHighTempLowWater->joinWithAND(wetHighTempLowWater, lowWater);

  FuzzyRuleAntecedent *wetHighTempMediumWater = new FuzzyRuleAntecedent();
  wetHighTempMediumWater->joinWithAND(wet, highTemp);
  FuzzyRuleAntecedent *forwetHighTempMediumWater = new FuzzyRuleAntecedent();
  forwetHighTempMediumWater->joinWithAND(wetHighTempMediumWater, mediumWater);

  FuzzyRuleAntecedent *wetHighTempHighWater = new FuzzyRuleAntecedent();
  wetHighTempHighWater->joinWithAND(wet, highTemp);
  FuzzyRuleAntecedent *forwetHighTempHighWater = new FuzzyRuleAntecedent();
  forwetHighTempHighWater->joinWithAND(wetHighTempHighWater, highWater);

  fuzzyWatering->addFuzzyRule(new FuzzyRule(19, forwetLowTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(20, forwetLowTempMediumWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(21, forwetLowTempHighWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(22, forwetMediumTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(23, forwetMediumTempMediumWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(24, forwetMediumTempHighWater, thenMediumDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(25, forwetHighTempLowWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(26, forwetHighTempMediumWater, thenShortDuration));
  fuzzyWatering->addFuzzyRule(new FuzzyRule(27, forwetHighTempHighWater, thenMediumDuration));
}
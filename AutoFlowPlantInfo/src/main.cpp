#include <Arduino.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>

#define DHT_PIN 3
#define DHT_TYPE DHT11

#define AOUT_PIN A6
#define THRESHOLD 530

DHT dht(DHT_PIN, DHT_TYPE);
float temperature = 0;

void readTemp();

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    dht.begin();
}

void loop() {
    // put your main code here, to run repeatedly:
    int value = analogRead(AOUT_PIN); // read the analog value from sensor
    readTemp();

    if (value > THRESHOLD)
      Serial.print("The soil is DRY (");
    else
      Serial.print("The soil is WET (");

    Serial.print(value);
    Serial.println(")");
    Serial.print("Temperature: ");
    Serial.println(temperature);
    delay(5000);
}

void readTemp() {
    temperature = dht.readTemperature();
}
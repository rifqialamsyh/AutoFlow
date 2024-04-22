#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const int trigPin = 2; // Ultrasonic sensor trigger pin
const int echoPin = 4; // Ultrasonic sensor echo pin
const float tankHeight = 15; // Height of the water tank in cm
const float tankVolume = 2.7; // Total volume of the water tank in liters

RF24 radio(7, 8); // CE, CSN pins

struct UltrasonicData {
  float depth;
};

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  radio.begin();
  radio.openWritingPipe(0xF0F0F0F0E2LL); // Set a different address for communication
  radio.setPALevel(RF24_PA_LOW);        // Set power level
}

void loop() {
  // Ultrasonic sensor operations
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  float duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2.0; 
  float actualDistance = distance - 2.5;// Calculate distance in cm

  // Calculate depth of water
  float depth = tankHeight - actualDistance;

  // Convert depth to liters
  float waterVolume = (depth * tankVolume) / tankHeight;
  float tuningWaterVolume = waterVolume - 0.15; // Tuning the water volume

  // Bundle the ultrasonic data
  UltrasonicData ultrasonicData;
  ultrasonicData.depth = tuningWaterVolume;

  // Send the data via NRF24L01
  radio.write(&ultrasonicData, sizeof(ultrasonicData));
  Serial.print("Water volume:");
  Serial.print(ultrasonicData.depth);
  Serial.println(" liters");

  Serial.print("Distance:");
  Serial.println(actualDistance);

  // Wait before sending again
  delay(1000); // Delay for 1 second
}

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const int trigPin = 2; // Ultrasonic sensor trigger pin
const int echoPin = 4; // Ultrasonic sensor echo pin

RF24 radio(7, 8); // CE, CSN pins

struct UltrasonicData {
  float distance;
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
  float distance = duration * 0.034 / 2; // Calculate distance in cm

  // Bundle the ultrasonic data
  UltrasonicData ultrasonicData;
  ultrasonicData.distance = distance;

  // Send the data via NRF24L01
  radio.write(&ultrasonicData, sizeof(ultrasonicData));
  Serial.print("Distance:");
  Serial.println(ultrasonicData.distance);
  // Wait before sending again
  delay(1000); // Delay for 2 seconds
}

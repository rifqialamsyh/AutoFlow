#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 5); // CE, CSN

const byte address[6] = "00001";

struct SensorValues
{
  char textValue;
  // int airValue;
  // float fuzzyValue;
};

SensorValues receivedData;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening(); // Set as receiver
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    Serial.println("============================");
    Serial.print("Message Received: ");
    Serial.println(receivedData.textValue);
  }
}

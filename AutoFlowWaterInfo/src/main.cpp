#include <SPI.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN

const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening(); // Set as transmitter
}

void loop() {
  const char text[] = "Hello World";
  radio.write(&text, sizeof(text));
  
  Serial.println("Sending message: ");
  Serial.println(text);

  delay(1000);
}

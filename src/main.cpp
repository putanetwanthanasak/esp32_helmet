#include <Arduino.h>

// put function declarations here:
const int ledPin = 5;


void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(ledPin,HIGH);
  delay(1000);
  digitalWrite(ledPin,LOW);
  delay(1000);
  Serial.println("test");
}


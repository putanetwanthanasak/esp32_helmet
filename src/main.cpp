#include <Arduino.h>

const int buzzerPin =  15;

void setup() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH); // ปิดเสียงตอนเริ่ม
  Serial.begin(115200);
  Serial.println("Active Buzzer Ready");
}

void loop() {
  // เปิดเสียง
  digitalWrite(buzzerPin, LOW);
  Serial.println("Buzzer ON");
  delay(1000);

  // ปิดเสียง
  digitalWrite(buzzerPin, HIGH);
  Serial.println("Buzzer OFF");
  delay(1000);
}
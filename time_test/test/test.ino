#include "Time.h"

void setup() {
  Serial.begin(9600);
  Serial.println("test");
  pinMode(7, OUTPUT);
}



void loop() {
  digitalWrite(7, HIGH);
}
#include <Arduino.h>
#include <HardwareSerial.h>

#include "./protocoll/index.hpp"

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
  }
  Serial.println("Starting...");

  PhysikalNode node;

  node.connections.push_back(25);

  node.start();

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
}

void loop() {}
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

  node.logicalNode.you.push_back(1);
  node.logicalNode.you.push_back(2);
  node.logicalNode.you.push_back(3);

  node.connections.push_back(25);

  node.start();

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
}

void loop() {}

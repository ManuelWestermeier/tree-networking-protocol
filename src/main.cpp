#include <Arduino.h>
#include <HardwareSerial.h>
#include "./protocoll/index.hpp"

#include "./user.hpp"

PhysikalNode node;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
  }

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println(String("Starting... ") + (IS_SENDER ? "SENDER" : "RECEIVER"));

  Address u1;
  u1.push_back(1);
  u1.push_back(2);
  u1.push_back(3);

  Address u2;
  u2.push_back(1);
  u2.push_back(27);

#if IS_SENDER
  node.logicalNode.you = u1;
  node.logicalNode.connections.push_back({u2, 25});
#else
  node.logicalNode.you = u2;
  node.logicalNode.connections.push_back({u1, 25});
#endif
  node.start();
#if IS_SENDER
  delay(5000);
  node.send(u2, "HELLO_____");
#endif
}

void loop() {}
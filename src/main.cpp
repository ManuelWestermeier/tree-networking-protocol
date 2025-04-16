#include <Arduino.h>
#include "./protocoll/index.hpp"

PhysikalNode node;

Address u1;
Address u2;
Address u3;
Address u4;

void setup()
{
  Serial.begin(115200);

  // User 1
  u1.push_back(1);
  // User 2
  u2.push_back(1);
  u2.push_back(27);
  // User 3
  u3.push_back(1);
  u3.push_back(27);
  u3.push_back(3);
  // User 4
  u4.push_back(1);
  u4.push_back(27);
  u4.push_back(4);

  node.onData = [](const char *data)
  {
    Serial.println("\nReceived:");
    Serial.println(data);
    Serial.println();
  };

  node.onError = [](String error)
  {
    Serial.println("\nError:");
    Serial.println(error);
    Serial.println();
  };

  Serial.println("U2");
  node.logicalNode.you = u2;
  node.logicalNode.connections.push_back({u1, 25});
  node.logicalNode.connections.push_back({u3, 26});

  node.start();

  // delay(5000);
  // node.send(u2, "1234567890");
}

void loop()
{
}

#include <Arduino.h>
#include "./protocoll/index.hpp"

#include "./user.hpp"

PhysikalNode node;

Address u1;
Address u2;
Address u3;
Address u4;

#define SENDER USER == 1

void setup()
{
  Serial.println("User: U" + String(USER));
  if (SENDER)
  {
    Serial.println("Sender");
  }

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
    digitalWrite(2, HIGH);
    Serial.println("\nReceived:");
    Serial.println(data);
    Serial.println();
    delay(100);
    digitalWrite(2, LOW);
  };

  node.onError = [](String error)
  {
    digitalWrite(2, HIGH);
    Serial.println("\nError:");
    Serial.println(error);
    Serial.println();
  };

#if USER == 1
  node.logicalNode.you = u1;
  node.logicalNode.connections.push_back({u2, 25});
#endif

#if USER == 2
  node.logicalNode.you = u2;
  node.logicalNode.connections.push_back({u1, 25});
  node.logicalNode.connections.push_back({u3, 26});
#endif

#if USER == 3
  node.logicalNode.you = u3;
  node.logicalNode.connections.push_back({u2, 26});
  node.logicalNode.connections.push_back({u4, 25});
#endif

#if USER == 4
  node.logicalNode.you = u4;
  node.logicalNode.connections.push_back({u3, 25});
#endif

  node.start();

#if SENDER
  delay(1000);
  node.send(u4, "HALLOWELT0");
#endif
}

void loop()
{
}

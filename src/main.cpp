#include <Arduino.h>
#include <HardwareSerial.h>
#include "./protocoll/index.hpp"

#include "./user.hpp"

// Global instance so that the node persists as long as the program runs.
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

  // Define two addresses.
  Address u1;
  u1.push_back(1);
  u1.push_back(2);
  u1.push_back(3);

  Address u2;
  u2.push_back(1);
  u2.push_back(27);

  // Set up logical node addresses and connections based on whether this node is a sender or receiver.
#if IS_SENDER
  node.logicalNode.you = u1;
  node.logicalNode.connections.push_back({u2, 25});
#else
  node.logicalNode.you = u2;
  node.logicalNode.connections.push_back({u1, 25});
#endif

  // Start the background task (the physical layer task that handles sending/receiving)
  node.start();

  // If this is the sender, send a packet after a short delay.
#if IS_SENDER
  delay(5000);
  node.send(u2, "HELLO_____");
#endif
}

void loop()
{
  // Nothing needed here as the node's task handles everything.
}

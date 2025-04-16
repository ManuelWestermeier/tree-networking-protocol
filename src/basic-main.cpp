// #include <Arduino.h>
// #include "./protocoll/index.hpp"
// #include "./user.hpp"

// PhysikalNode node;

// Address u1;
// Address u2;
// Address u3;

// void setup()
// {
//     Serial.begin(115200);

//     Serial.println();
//     Serial.println("Starting...");
//     Serial.println(String("Role: ") + (IS_SENDER ? "SENDER" : "RECEIVER"));

//     // User 1
//     u1.push_back(1);
//     u1.push_back(2);
//     u1.push_back(3);
//     // User 2
//     u2.push_back(1);
//     u2.push_back(27);
//     // User 3
//     u3.push_back(0);

//     node.onData = [](const char *data)
//     {
//         Serial.println("\nReceived:");
//         Serial.println(data);
//         Serial.println();
//     };

//     node.onError = [](String error)
//     {
//         Serial.println("\nError:");
//         Serial.println(error);
//         Serial.println();
//     };

// #if IS_SENDER
//     node.logicalNode.you = u1;
//     node.logicalNode.connections.push_back({u2, 25});
// #else
//     node.logicalNode.you = u2;
//     node.logicalNode.connections.push_back({u1, 25});
// #endif

//     node.start();
// }

// void loop()
// {
// #if IS_SENDER
//     delay(5000);
//     node.send(u2, "HEY  HEY   ");
//     delay(20000);
// #endif
// }

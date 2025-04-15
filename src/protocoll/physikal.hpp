#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "logical.hpp"

#define NORMAL_SEND 1

struct PhysikalNode
{
  vector<uint8_t> connections;
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;

  static void loopTask(void *params)
  {
    static_cast<PhysikalNode *>(params)->loop();
  }

  uint8_t readByte(uint8_t pin)
  {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
      value |= (digitalRead(pin) << (7 - i));
      delayMicroseconds(100);
    }
    return value;
  }

  uint16_t readUInt16(uint8_t pin)
  {
    uint8_t low = readByte(pin);
    uint8_t high = readByte(pin);
    return (high << 8) | low;
  }

  void receivePocket(uint8_t pin)
  {
    uint8_t pocketType = readByte(pin);

    if (pocketType == NORMAL_SEND)
    {
      uint16_t addrSize = readUInt16(pin);
      Address address;

      for (int i = 0; i < addrSize; i++)
      {
        address.push_back(readUInt16(pin));
      }

      char data[11];
      for (int i = 0; i < 10; i++)
      {
        data[i] = readByte(pin);
      }
      data[10] = '\0';

      uint16_t checksum = readUInt16(pin);

      Pocket p(address, data);
      if (p.checksum != checksum)
      {
        Serial.println("Checksum mismatch!");
        return;
      }
      on(p);
    }
    else if (pocketType == RETURN_OK)
    {
      // read pocket hash
      // delete hash form the hashmap with the sended pocjets (to ensure, that a pocket is good)
    }
  }

  void sendByte(uint8_t pin, uint8_t byte)
  {
    for (int i = 7; i >= 0; i--)
    {
      digitalWrite(pin, (byte >> i) & 1);
      delayMicroseconds(100);
    }
  }

  void sendUInt16(uint8_t pin, uint16_t val)
  {
    sendByte(pin, val & 0xFF);
    sendByte(pin, val >> 8);
  }

  void sendNormalPocket(Pocket &p, uint8_t pin)
  {
    pinMode(pin, OUTPUT);

    sendByte(pin, NORMAL_SEND);
    sendUInt16(pin, p.address.size());

    for (auto a : p.address)
      sendUInt16(pin, a);

    for (int i = 0; i < 10; i++)
      sendByte(pin, p.data[i]);

    sendUInt16(pin, p.checksum);

    pinMode(pin, INPUT); // allow receiving again
  }

  void loop()
  {
    // Pulse each connection to signal presence
    for (auto pin : connections)
    {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      delayMicroseconds(1000);
      digitalWrite(pin, LOW);
      pinMode(pin, INPUT);
    }

    while (true)
    {
      for (auto pin : connections)
      {
        if (digitalRead(pin) == HIGH)
        {
          delayMicroseconds(100);
          receivePocket(pin);
        }
      }
      delayMicroseconds(100);
    }
  }

  void on(Pocket p)
  {
    uint8_t sendPin = logicalNode.recieve(p);
    if (sendPin == 0)
    {
      Serial.print("Received: ");
      Serial.println(p.data);
    }
    else
    {
      sendNormalPocket(p, sendPin);
    }
  }

  void start()
  {
    if (taskHandle == nullptr)
    {
      xTaskCreate(loopTask, "PhysLoop", 4096, this, 1, &taskHandle);
    }
  }

  void stop()
  {
    if (taskHandle != nullptr)
    {
      vTaskDelete(taskHandle);
      taskHandle = nullptr;
    }
  }
};
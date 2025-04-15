#pragma once

#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <string.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "./logical.hpp"

using namespace std;

struct PhysikalPocket
{
  uint8_t pocketType;
  Pocket logicalPocket;
};

#define NORMAL_SEND 1

struct PhysikalNode
{
  vector<uint8_t> connections;
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;

  static void loopTask(void *params)
  {
    PhysikalNode *instance = static_cast<PhysikalNode *>(params);
    instance->loop();
  }

  uint8_t readByte(uint8_t pin)
  {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
      value <<= 1;
      value |= digitalRead(pin);
      delayMicroseconds(100);
    }
    return value;
  }

  uint16_t readUInt16(uint8_t pin)
  {
    uint16_t low = readByte(pin);
    uint16_t high = readByte(pin);
    return (high << 8) | low;
  }

  void receivePocket(uint8_t pin)
  {
    uint8_t pocketType = readByte(pin);
    uint8_t addrSize = readUInt16(pin);

    Address address;
    address.reserve(addrSize);

    for (int i = 0; i < addrSize; i++)
    {
      address.push_back(readUInt16(pin));
    }

    uint8_t data[10];
    for (int i = 0; i < 10; i++)
    {
      data[i] = readByte(pin);
    }

    uint16_t checksum = readUInt16(pin);

    const char *d = (char *)data;
    Pocket logicalPocket(address, d);

    if (logicalPocket.checksum != checksum)
    {
      Serial.println("Error: pp.logicalPocket.checksum != checksum");
      return;
    }

    Serial.println("Packet Received");

    if (pocketType == NORMAL_SEND)
      on(logicalPocket.address, logicalPocket.data);
  }

  void loop()
  {
    for (const auto &connection : connections)
    {
      pinMode(connection, OUTPUT);
      digitalWrite(connection, HIGH);
      delayMicroseconds(1000);
      digitalWrite(connection, LOW);
      pinMode(connection, INPUT);
    }

    while (true)
    {
      for (const auto &connection : connections)
      {
        if (digitalRead(connection) == HIGH)
        {
          delayMicroseconds(100);
          receivePocket(connection);
        }
      }
      delayMicroseconds(100);
    }
  }

  void sendNoramlPocket(Pocket p, uint8_t pin)
  {
  }

  void on(Address to, const char *data)
  {
    auto p = Pocket(to, data);
    uint8_t sendPin = logicalNode.recieve(p);

    if (!sendPin)
    {
      Serial.print("Data Recieved: ");
      Serial.println(String(p.data));
      return;
    }

    sendNoramlPocket(p, sendPin);
  }

  void start()
  {
    if (taskHandle == nullptr)
    {
      xTaskCreate(
          loopTask,
          "Protocol Task",
          2048,
          this,
          1,
          &taskHandle);
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

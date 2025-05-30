#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

#include "./raw-communication.hpp"
#include "logical.hpp"
#include "pending-packet.hpp"

#define NORMAL_SEND 1
#define RETURN_OK 2

#define RESEND_TIMEOUT 5000 // milliseconds
#define MAX_ATTEMPTS 50

using std::vector;

struct PhysikalNode
{
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;
  vector<PendingPacket> pendingPackets;

  std::function<void(const char *data)> onData = nullptr;
  std::function<void(String)> onError = nullptr;

  static void loopTask(void *params)
  {
    static_cast<PhysikalNode *>(params)->loop();
  }

  void receivePocket(uint8_t pin);

  void sendNormalPocket(Pocket &p, uint8_t pin);

  void on(Pocket p)
  {
    Serial.println("[Protocol] on: handling received pocket");

    uint8_t sendPin = logicalNode.recieve(p);

    if (sendPin == 0)
    {
      Serial.println("[Protocol] on: delivering data to application layer");
      onData(p.data);
    }
    else if (sendPin == -1)
    {
      Serial.println("[Protocol] on: pocket cannot reach destination");
      onError("pocket cannot reach destination");
    }
    else
    {
      Serial.print("[Protocol] on: forwarding pocket via pin ");
      Serial.println(sendPin);
      sendNormalPocket(p, sendPin);
    }
  }

  void acknowledge(uint16_t hash)
  {
    Serial.print("[Protocol] acknowledge: checking for hash ");
    Serial.println(hash);

    for (auto it = pendingPackets.begin(); it != pendingPackets.end(); ++it)
    {
      if (it->pocket.checksum == hash)
      {
        Serial.println("[Protocol] acknowledge: match found, removing pending packet");
        pendingPackets.erase(it);
        break;
      }
    }
  }

  void checkPendingAcks();
  void handlePacketRetry(PendingPacket &pending, unsigned long currentTime);
  void handlePacketFailure(vector<PendingPacket>::iterator &it, unsigned long currentTime);

  void loop()
  {
    Serial.println("[Protocol] loop: starting main loop");

    for (auto conn : logicalNode.connections)
    {
      pinMode(conn.pin, INPUT);
    }

    while (true)
    {
      for (const auto &conn : logicalNode.connections)
      {
        if (digitalRead(conn.pin) == HIGH)
        {
          delayMicroseconds(BIT_DELAY * 1.4);
          receivePocket(conn.pin);
        }
      }
      checkPendingAcks();
      vTaskDelay(0);
    }
  }

  void start()
  {
    if (taskHandle == nullptr)
    {
      Serial.println("[Protocol] start: creating FreeRTOS task");
      xTaskCreate(loopTask, "PhysLoop", 4096, this, 1, &taskHandle);
    }
  }

  void stop()
  {
    if (taskHandle != nullptr)
    {
      Serial.println("[Protocol] stop: deleting FreeRTOS task");
      vTaskDelete(taskHandle);
      taskHandle = nullptr;
    }
  }

  void send(Address address, const char *data)
  {
    Serial.println("[Protocol] send: creating and sending pocket");
    on(Pocket(address, data));
  }
};

#include "./raw-communication.hpp"
#include "./check-pending-acks.hpp"
#include "./receive-pocket.hpp"
#include "./send-normal-pocket.hpp"

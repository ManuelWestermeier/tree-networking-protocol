#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

#include "logical.hpp"

#define NORMAL_SEND 1
#define RETURN_OK 2
#define RESEND_TIMEOUT 5000 // milliseconds
#define MAX_ATTEMPTS 3

using std::vector;

struct PendingPacket
{
  Pocket pocket;
  uint8_t sendPin;
  uint8_t attempts;
  unsigned long lastSendTime; // in milliseconds

  PendingPacket(const Pocket &p, uint8_t pin, uint8_t att, unsigned long time)
      : pocket(p), sendPin(pin), attempts(att), lastSendTime(time) {}
};

struct PhysikalNode
{
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;
  vector<PendingPacket> pendingPackets;

  static void loopTask(void *params)
  {
    static_cast<PhysikalNode *>(params)->loop();
  }

  void receivePocket(uint8_t pin);

  void sendNormalPocket(Pocket &p, uint8_t pin);

  std::function<void(const char *data)> onData = nullptr;

  // Handles a packet once received.
  void on(Pocket p)
  {
    uint8_t sendPin = logicalNode.recieve(p);
    if (sendPin == 0)
    {
      onData(p.data);
    }
    else
    {
      sendNormalPocket(p, sendPin);
    }
  }

  // Finds and acknowledges the pending packet based on its checksum.
  void acknowledge(uint16_t hash)
  {
    for (auto it = pendingPackets.begin(); it != pendingPackets.end(); ++it)
    {
      if (it->pocket.checksum == hash)
      {
        pendingPackets.erase(it);
        break;
      }
    }
  }

  // Checks for packets that need to be resent.
  void checkPendingAcks();

  // The main loop—pulses the connections and checks for incoming data or ACKs.
  void loop()
  {
    // Pulse each connection to signal presence (adjust as needed)
    for (auto conn : logicalNode.connections)
    {
      // Uncomment to use pulsing signals if needed:
      // pinMode(conn.pin, OUTPUT);
      // digitalWrite(conn.pin, HIGH);
      // delayMicroseconds(1000);
      // digitalWrite(conn.pin, LOW);
      pinMode(conn.pin, INPUT);
    }

    while (true)
    {
      for (auto conn : logicalNode.connections)
      {
        if (digitalRead(conn.pin) == HIGH)
        {
          delayMicroseconds(1500);
          receivePocket(conn.pin);
        }
      }
      checkPendingAcks();
      // Yield to other tasks to avoid watchdog resets.
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }

  // Starts the FreeRTOS task for handling the physical layer communications.
  void start()
  {
    if (taskHandle == nullptr)
    {
      xTaskCreate(loopTask, "PhysLoop", 4096, this, 1, &taskHandle);
    }
  }

  // Stops the FreeRTOS task, if running.
  void stop()
  {
    if (taskHandle != nullptr)
    {
      vTaskDelete(taskHandle);
      taskHandle = nullptr;
    }
  }

  // Initiates the sending of a packet with the given address and data.
  void send(Address address, const char *data)
  {
    on(Pocket(address, data));
  }
};

#include "./raw-communication.hpp"
#include "./check-pending-acks.hpp"
#include "./receive-pocket.hpp"
#include "./send-normal-pocket.hpp"

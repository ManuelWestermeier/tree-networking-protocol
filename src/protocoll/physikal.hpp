#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

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

  // Handles a packet once received.
  void on(Pocket p)
  {
    uint8_t sendPin = logicalNode.recieve(p);

    // 0 == Selv Send
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
  void handlePacketRetry(PendingPacket &pending, unsigned long currentTime);
  void handlePacketFailure(vector<PendingPacket>::iterator &it, unsigned long currentTime);

  // The main loop—pulses the connections and checks for incoming data or ACKs.
  void loop()
  {
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
          delayMicroseconds(1500);
          // wait on bit + a half (read in the middle of the signal)
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

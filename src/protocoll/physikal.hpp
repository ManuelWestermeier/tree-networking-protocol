#pragma once

#include <Arduino.h>
#include <vector>
#include <queue>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "./raw-communication.hpp"
#include "logical.hpp"

#define NORMAL_SEND 1
#define RETURN_OK 2

#define RESEND_TIMEOUT 5000 // milliseconds
#define MAX_ATTEMPTS 50

using std::vector;

struct SendRequest
{
  Pocket pocket;
  uint8_t pin;
  SendRequest(const Pocket &p, uint8_t pin_) : pocket(p), pin(pin_) {}
};

struct PhysikalNode
{
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;
  QueueHandle_t sendQueue = nullptr;

  std::function<void(Pocket pocket)> onData = nullptr;
  std::function<void(String error, Pocket pocket)> onError = nullptr;

  static void loopTask(void *params)
  {
    static_cast<PhysikalNode *>(params)->loop();
  }

  void receivePocket(uint8_t pin);
  void sendNormalPocket(Pocket &p, uint8_t pin);

  // ---- Queue helpers ----
  bool enqueueSend(const Pocket &p, uint8_t pin)
  {
    if (!sendQueue)
      return false;
    SendRequest *req = new SendRequest(p, pin);
    BaseType_t ok = xQueueSend(sendQueue, &req, pdMS_TO_TICKS(50));
    if (ok != pdTRUE)
    {
      delete req;
      return false;
    }
    return true;
  }

  void on(Pocket p)
  {
    Serial.println("[Protocol] on: handling received pocket");

    uint8_t sendPin = logicalNode.recieve(p);

    if (sendPin == 0)
    {
      Serial.println("[Protocol] on: delivering data to application layer");
      if (onData)
        onData(p);
    }
    else if (sendPin == (uint8_t)-1)
    {
      Serial.println("[Protocol] on: pocket cannot reach destination");
      if (onError)
        onError("pocket cannot reach destination", p);
    }
    else
    {
      Serial.printf("[Protocol] on: forwarding pocket via pin %u\n", sendPin);
      enqueueSend(p, sendPin);
    }
  }

  void loop()
  {
    Serial.println("[Protocol] loop: starting main loop");

    for (auto conn : logicalNode.connections)
    {
      pinMode(conn.pin, INPUT_PULLDOWN); // stabiler gegen Rauschen
    }

    while (true)
    {
      // 1) process queued sends
      if (sendQueue)
      {
        SendRequest *req = nullptr;
        if (xQueueReceive(sendQueue, &req, pdMS_TO_TICKS(1)) == pdTRUE)
        {
          if (req)
          {
            sendNormalPocket(req->pocket, req->pin);
            delete req;
          }
        }
      }

      // 2) check for incoming
      for (const auto &conn : logicalNode.connections)
      {
        if (digitalRead(conn.pin) == HIGH)
        {
          delayMicroseconds(BIT_DELAY * 1.4);
          receivePocket(conn.pin);
        }
      }

      vTaskDelay(pdMS_TO_TICKS(1)); // yield
    }
  }

  void start()
  {
    if (taskHandle == nullptr)
    {
      Serial.println("[Protocol] start: creating FreeRTOS task + queue");
      sendQueue = xQueueCreate(8, sizeof(SendRequest *));
      xTaskCreate(loopTask, "PhysLoop", 8192, this, 1, &taskHandle);
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
    if (sendQueue)
    {
      vQueueDelete(sendQueue);
      sendQueue = nullptr;
    }
  }

  void send(Address address, const char *data)
  {
    Serial.println("[Protocol] send: creating and enqueueing pocket");
    auto p = Pocket(address, data);
    p.id = random(65535);
    // Erst an logicalNode geben, entscheidet Pin oder local
    uint8_t sendPin = logicalNode.send(p);
    if (sendPin == 0)
    {
      if (onData)
        onData(p);
    }
    else if (sendPin == (uint8_t)-1)
    {
      if (onError)
        onError("pocket cannot reach destination", p);
    }
    else
    {
      enqueueSend(p, sendPin);
    }
  }
};

#include "./raw-communication.hpp"
#include "./receive-pocket.hpp"
#include "./send-normal-pocket.hpp"

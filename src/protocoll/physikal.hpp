#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "logical.hpp"

#define NORMAL_SEND 1
#define RETURN_OK 2
#define RESEND_TIMEOUT 5000UL // milliseconds
#define MAX_ATTEMPTS 3

using std::vector;

// Custom structure to hold a pending packet with its current state.
struct PendingPacket
{
  Pocket pocket;
  uint8_t sendPin;
  uint8_t attempts;
  unsigned long lastSendTime; // in milliseconds

  // Custom constructor to initialize all members.
  PendingPacket(const Pocket &p, uint8_t pin, uint8_t att, unsigned long time)
      : pocket(p), sendPin(pin), attempts(att), lastSendTime(time) {}
};

struct PhysikalNode
{
  Node logicalNode;
  TaskHandle_t taskHandle = nullptr;
  vector<PendingPacket> pendingPackets; // pending packets waiting for acknowledgment

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
      // For an acknowledgment, read the 16-bit hash.
      uint16_t hash = readUInt16(pin);
      // Process the acknowledgment by removing the matching pending packet.
      acknowledge(hash);
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

    pinMode(pin, INPUT); // switch back to receive mode

    // Only add the packet if it doesn't already exist in pendingPackets
    bool exists = false;
    for (auto &pending : pendingPackets)
    {
      if (pending.pocket.checksum == p.checksum)
      {
        exists = true;
        break;
      }
    }
    if (!exists)
    {
      // Use the custom constructor to add the pending packet.
      pendingPackets.push_back(PendingPacket(p, pin, 1, millis()));
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

  void acknowledge(uint16_t hash)
  {
    for (auto it = pendingPackets.begin(); it != pendingPackets.end(); ++it)
    {
      if (it->pocket.checksum == hash)
      {
        Serial.print("ACK received for packet: ");
        Serial.println(hash, HEX);
        pendingPackets.erase(it);
        break;
      }
    }
  }

  void checkPendingAcks()
  {
    unsigned long currentTime = millis();
    for (size_t i = 0; i < pendingPackets.size();)
    {
      PendingPacket &pending = pendingPackets[i];
      if (currentTime - pending.lastSendTime > RESEND_TIMEOUT)
      {
        if (pending.attempts < MAX_ATTEMPTS)
        {
          pending.attempts++;
          pending.lastSendTime = currentTime;
          Serial.print("Resending packet ");
          Serial.print(pending.pocket.checksum, HEX);
          Serial.print(" attempt ");
          Serial.println(pending.attempts);
          sendNormalPocket(pending.pocket, pending.sendPin);
          ++i;
        }
        else
        {
          Serial.print("Packet ");
          Serial.print(pending.pocket.checksum, HEX);
          Serial.println(" failed 3 times. Removing connection and retrying on a new pin.");
          uint8_t failedPin = pending.sendPin;
          for (auto it = logicalNode.connections.begin(); it != logicalNode.connections.end(); ++it)
          {
            if (it->pin == failedPin)
            {
              logicalNode.connections.erase(it);
              break;
            }
          }
          if (!logicalNode.connections.empty())
          {
            uint8_t newPin = logicalNode.connections[0].pin; // choose a new available connection
            Serial.print("Resending on new connection pin: ");
            Serial.println(newPin);
            pending.sendPin = newPin;
            pending.attempts = 1;
            pending.lastSendTime = currentTime;
            sendNormalPocket(pending.pocket, newPin);
            ++i;
          }
          else
          {
            Serial.println("No available logicalNode.connections to resend packet.");
            pendingPackets.erase(pendingPackets.begin() + i);
          }
        }
      }
      else
      {
        ++i;
      }
    }
  }

  void loop()
  {
    // Pulse each connection to signal presence.
    for (auto conn : logicalNode.connections)
    {
      pinMode(conn.pin, OUTPUT);
      digitalWrite(conn.pin, HIGH);
      delayMicroseconds(1000);
      digitalWrite(conn.pin, LOW);
      pinMode(conn.pin, INPUT);
    }

    while (true)
    {
      for (auto conn : logicalNode.connections)
      {
        if (digitalRead(conn.pin) == HIGH)
        {
          delayMicroseconds(100);
          receivePocket(conn.pin);
        }
      }
      checkPendingAcks();
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

  void send(Address address, const char *data)
  {
    on(Pocket(address, data));
  }
};

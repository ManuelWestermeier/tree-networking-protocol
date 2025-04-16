#pragma once

#include "./physikal.hpp"
#include <algorithm>

void PhysikalNode::checkPendingAcks()
{
    unsigned long currentTime = millis();
    for (auto it = pendingPackets.begin(); it != pendingPackets.end();)
    {
        PendingPacket &pending = *it;
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
                ++it;
            }
            else
            {
                Serial.print("Packet ");
                Serial.print(pending.pocket.checksum, HEX);
                Serial.println(" failed 3 times. Removing connection and retrying on a new pin.");
                uint8_t failedPin = pending.sendPin;
                // Erase the failed connection using std::find_if.
                auto connIt = std::find_if(
                    logicalNode.connections.begin(),
                    logicalNode.connections.end(),
                    [failedPin](const Connection &conn)
                    { return conn.pin == failedPin; });
                if (connIt != logicalNode.connections.end())
                {
                    logicalNode.connections.erase(connIt);
                }
                // If an alternative connection exists, update and resend.
                if (!logicalNode.connections.empty())
                {
                    uint8_t newPin = logicalNode.connections.front().pin;
                    Serial.print("Resending on new connection pin: ");
                    Serial.println(newPin);
                    pending.sendPin = newPin;
                    pending.attempts = 1;
                    pending.lastSendTime = currentTime;
                    sendNormalPocket(pending.pocket, newPin);
                    ++it;
                }
                else
                {
                    Serial.println("No available logicalNode.connections to resend packet.");
                    it = pendingPackets.erase(it);
                }
            }
        }
        else
        {
            ++it;
        }
    }
}
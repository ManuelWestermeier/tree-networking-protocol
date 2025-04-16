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
                if (onError != nullptr)
                {
                    onError(String("Checksum mismatch!"));
                    onError(String("Resending packet "));
                    onError(String(pending.pocket.checksum, HEX));
                    onError(String(" attempt "));
                    onError(String(pending.attempts));
                }
                sendNormalPocket(pending.pocket, pending.sendPin);
                ++it;
            }
            else
            {
                if (onError != nullptr)
                {
                    onError(String("Packet "));
                    onError(String(pending.pocket.checksum, HEX));
                    onError(String(" failed 3 times. Removing connection and retrying on a new pin."));
                }
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
                    if (onError != nullptr)
                    {
                        onError(String("Resending on new connection pin: "));
                        onError(String(newPin));
                    }
                    pending.sendPin = newPin;
                    pending.attempts = 1;
                    pending.lastSendTime = currentTime;
                    sendNormalPocket(pending.pocket, newPin);
                    ++it;
                }
                else
                {
                    if (onError != nullptr)
                        onError(String("No available logicalNode.connections to resend packet."));
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
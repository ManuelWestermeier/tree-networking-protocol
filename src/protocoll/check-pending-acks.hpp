#pragma once

#include "./physikal.hpp"
#include "./pending-packet.hpp"

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
                handlePacketRetry(pending, currentTime);
                ++it;
            }
            else
            {
                handlePacketFailure(it, currentTime);
            }
        }
        else
        {
            ++it;
        }
    }
}

void PhysikalNode::handlePacketRetry(PendingPacket &pending, unsigned long currentTime)
{
    pending.attempts++;
    pending.lastSendTime = currentTime;

    if (onError != nullptr)
    {
        onError("Checksum mismatch! Resending packet " +
                String(pending.pocket.checksum, HEX) +
                " attempt " + String(pending.attempts));
    }

    sendNormalPocket(pending.pocket, pending.sendPin);
}

void PhysikalNode::handlePacketFailure(vector<PendingPacket>::iterator &it, unsigned long currentTime)
{
    PendingPacket &pending = *it;

    if (onError != nullptr)
    {
        onError("Packet " + String(pending.pocket.checksum, HEX) +
                " failed 3 times. Removing connection and retrying on a new pin.");
    }

    uint8_t failedPin = pending.sendPin;

    auto connIt = std::find_if(
        logicalNode.connections.begin(),
        logicalNode.connections.end(),
        [failedPin](const Connection &conn)
        { return conn.pin == failedPin; });

    if (connIt != logicalNode.connections.end())
    {
        logicalNode.connections.erase(connIt);
    }

    if (!logicalNode.connections.empty())
    {
        uint8_t newPin = logicalNode.connections.front().pin;

        if (onError != nullptr)
        {
            onError("Resending on new connection pin: " + String(newPin));
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
        {
            onError("No available logicalNode.connections to resend packet.");
        }

        it = pendingPackets.erase(it);
    }
}

#pragma once

#include "./physikal.hpp"
#include "./pending-packet.hpp"

#include <algorithm>

void PhysikalNode::checkPendingAcks()
{
    unsigned long currentTime = millis();
    Serial.println("[Protocol] checkPendingAcks: checking for expired ACKs");

    for (auto it = pendingPackets.begin(); it != pendingPackets.end();)
    {
        PendingPacket &pending = *it;

        if (currentTime - pending.lastSendTime > RESEND_TIMEOUT)
        {
            if (pending.attempts < MAX_ATTEMPTS)
            {
                Serial.print("[Protocol] checkPendingAcks: retrying packet ");
                Serial.println(pending.pocket.checksum, HEX);
                handlePacketRetry(pending, currentTime);
                ++it;
            }
            else
            {
                Serial.print("[Protocol] checkPendingAcks: packet failed too often, handling failure ");
                Serial.println(pending.pocket.checksum, HEX);
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

    Serial.print("[Protocol] handlePacketRetry: attempt ");
    Serial.print(pending.attempts);
    Serial.print(" for packet ");
    Serial.println(pending.pocket.checksum, HEX);

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

    Serial.print("[Protocol] handlePacketFailure: packet ");
    Serial.print(pending.pocket.checksum, HEX);
    Serial.println(" failed, removing current connection");

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

        Serial.print("[Protocol] handlePacketFailure: retrying on pin ");
        Serial.println(newPin);

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
        Serial.println("[Protocol] handlePacketFailure: no more connections available, dropping packet");

        if (onError != nullptr)
        {
            onError("No available logicalNode.connections to resend packet.");
        }

        it = pendingPackets.erase(it);
    }
}

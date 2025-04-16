#pragma once

#include "./physikal.hpp"

void PhysikalNode::sendNormalPocket(Pocket &p, uint8_t pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(1000);

    sendByte(pin, NORMAL_SEND);
    sendUInt16(pin, p.address.size());
    for (auto a : p.address)
        sendUInt16(pin, a);
    for (int i = 0; i < 10; i++)
        sendByte(pin, p.data[i]);
    sendUInt16(pin, p.checksum);

    pinMode(pin, INPUT); // Switch back to receive mode

    // Add packet to pendingPackets only if it is not already pending
    bool alreadyPending = false;
    for (auto &pending : pendingPackets)
    {
        if (pending.pocket.checksum == p.checksum)
        {
            alreadyPending = true;
            break;
        }
    }
    if (!alreadyPending)
    {
        pendingPackets.push_back(PendingPacket(p, pin, 1, millis()));
    }
}
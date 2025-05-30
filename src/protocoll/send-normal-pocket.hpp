#pragma once

#include "./physikal.hpp"

void PhysikalNode::sendNormalPocket(Pocket &p, uint8_t pin)
{
    Serial.print("[Protocol] sendNormalPocket: sending on pin ");
    Serial.println(pin);

    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(1000);

    sendByte(pin, NORMAL_SEND);
    sendUInt16(pin, p.address.size());
    for (auto a : p.address)
    {
        sendUInt16(pin, a);
    }
    for (int i = 0; i < 10; i++)
    {
        sendByte(pin, p.data[i]);
    }

    sendUInt16(pin, p.checksum);

    pinMode(pin, INPUT); // Switch back to receive mode

    for (auto &pending : pendingPackets)
    {
        if (pending.pocket.checksum == p.checksum)
        {
            Serial.println("[Protocol] sendNormalPocket: packet already pending, not adding again.");
            return;
        }
    }

    Serial.print("[Protocol] sendNormalPocket: queued packet with checksum ");
    Serial.println(p.checksum, HEX);

    pendingPackets.push_back(PendingPacket(p, pin, 1, millis()));
}

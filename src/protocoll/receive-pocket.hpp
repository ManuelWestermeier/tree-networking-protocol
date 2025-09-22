#pragma once

#include "./physikal.hpp"

void PhysikalNode::receivePocket(uint8_t pin)
{
    uint16_t addrSize = readUInt16(pin);
    Address address;
    Serial.println("Addr size: " + String(addrSize));

    address.reserve(addrSize);

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
        Serial.println("[Protocol] receivePocket: checksum mismatch");

        if (onError != nullptr)
        {
            onError("Checksum mismatch! Data:");
            onError(data);
        }
        return;
    }

    Serial.println("[Protocol] receivePocket: checksum valid, sending ACK");
    on(p);
}
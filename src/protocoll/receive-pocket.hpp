#pragma once

#include "./physikal.hpp"

void PhysikalNode::receivePocket(uint8_t pin)
{
    Address address;

    while (true)
    {
        auto v = readUInt16(pin);
        if (v == 0)
            break;
        address.push_back(v);
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
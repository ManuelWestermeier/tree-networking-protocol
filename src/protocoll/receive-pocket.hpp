#pragma once

#include "./physikal.hpp"

void sendAck(uint8_t pin, uint16_t checksum)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(1000);

    sendByte(pin, RETURN_OK);
    sendUInt16(pin, checksum);

    pinMode(pin, INPUT);
}

void PhysikalNode::receivePocket(uint8_t pin)
{
    uint8_t pocketType = readByte(pin);

    if (pocketType == NORMAL_SEND)
    {
        uint16_t addrSize = readUInt16(pin);
        Address address;
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
            if (onError != nullptr)
            {
                onError("Checksum mismatch! Data:");
                onError(data);
            }
            return;
        }

        sendAck(pin, p.checksum);

        on(p);
    }
    else if (pocketType == RETURN_OK)
    {
        uint16_t hash = readUInt16(pin);
        acknowledge(hash);
    }
}
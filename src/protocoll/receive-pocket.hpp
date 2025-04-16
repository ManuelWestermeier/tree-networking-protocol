#pragma once

#include "./physikal.hpp"

void PhysikalNode::receivePocket(uint8_t pin)
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

        // Debug prints to verify checksum
        Serial.println("Received data: ");
        Serial.println(data);
        Serial.print("Computed checksum: ");
        Serial.println(p.checksum);
        Serial.print("Received checksum: ");
        Serial.println(checksum);

        if (p.checksum != checksum)
        {
            Serial.println("Checksum mismatch!");
            return;
        }

        pinMode(pin, OUTPUT);
        delayMicroseconds(1000);
        sendByte(pin, RETURN_OK);
        sendUInt16(pin, p.checksum);
        pinMode(pin, INPUT);

        on(p);
    }
    else if (pocketType == RETURN_OK)
    {
        uint16_t hash = readUInt16(pin);
        acknowledge(hash);
    }
}
#pragma once

#include "./physikal.hpp"

#define IGNORE_ID_POOL_SIZE 16
uint16_t ignorePoolIds[IGNORE_ID_POOL_SIZE];
size_t ignorePoolIndex = 0;

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

    char data[DATASIZE + 1];
    for (int i = 0; i < DATASIZE; i++)
    {
        data[i] = readByte(pin);
    }
    data[DATASIZE] = '\0';

    uint16_t id = readUInt16(pin);
    uint16_t checksum = readUInt16(pin);

    Pocket p(address, data);
    p.id = id;

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

    for (size_t i = 0; i < IGNORE_ID_POOL_SIZE; i++)
    {
        if (ignorePoolIds[i] == id)
        {
            Serial.println("[Protocol] receivePocket: duplicate pocket, ignoring");
            return;
        }
    }

    ignorePoolIds[ignorePoolIndex] = id;
    ignorePoolIndex = (ignorePoolIndex + 1) % IGNORE_ID_POOL_SIZE;

    Serial.println("[Protocol] receivePocket: checksum valid");
    on(p);
}
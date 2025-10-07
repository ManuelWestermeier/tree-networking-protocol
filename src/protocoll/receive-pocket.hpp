#pragma once

#include "./physikal.hpp"

#define IGNORE_ID_POOL_SIZE 16
uint16_t ignorePoolIds[IGNORE_ID_POOL_SIZE];
size_t ignorePoolIndex = 0;

void PhysikalNode::handleMenagementFrame(uint8_t pin)
{
    bool type = digitalRead(pin);
    delayMicroseconds(BIT_DELAY);

    Serial.println("Receaved Data Frame");
    Serial.println(type ? "Connect Request" : "Adress Request");

    if (type == 1) // Connect Request
    {
        // get Address
        Address address;

        while (true)
        {
            auto v = readUInt16(pin);
            if (v == 0)
                break;
            address.push_back(v);
        }

        // sen ok, adress back
        delayMicroseconds(BIT_DELAY);
        pinMode(pin, OUTPUT);
        // start = LOW, HIGH
        digitalWrite(pin, LOW);
        delayMicroseconds(BIT_DELAY);
        digitalWrite(pin, HIGH);
        delayMicroseconds(BIT_DELAY);

        bool ok = true;

        for (const auto &connection : logicalNode.connections)
        {
            if (eq(connection.address, address) || connection.pin == pin)
            {
                ok = false;
                break;
            }
        }

        digitalWrite(pin, ok);
        delayMicroseconds(BIT_DELAY);

        if (ok)
        {
            logicalNode.connections.push_back(Connection{address, pin});
        }

        // LOW = END
        delayMicroseconds(BIT_DELAY);
        digitalWrite(pin, LOW);
        pinMode(pin, INPUT);
    }

    if (type == 0) // Adress Request
    {
        // sen ok, adress back
        delayMicroseconds(BIT_DELAY);
        pinMode(pin, OUTPUT);
        // start = LOW, HIGH
        digitalWrite(pin, LOW);
        delayMicroseconds(BIT_DELAY);
        digitalWrite(pin, HIGH);
        delayMicroseconds(BIT_DELAY);

        // address
        for (auto a : logicalNode.you)
        {
            sendUInt16(pin, a);
        }
        sendUInt16(pin, 0); // End of address marker

        // LOW = END
        delayMicroseconds(BIT_DELAY);
        digitalWrite(pin, LOW);
        pinMode(pin, INPUT);
    }
}

void PhysikalNode::receivePocket(uint8_t pin)
{
    bool isDataFrame = digitalRead(pin);
    delayMicroseconds(BIT_DELAY);

    // Meanagement
    if (!isDataFrame)
    {
        handleMenagementFrame(pin);
    }

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
            onError("Checksum mismatch! Data:", p);
            onError(data, p);
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
#pragma once

#define BIT_DELAY 2000

#include <Arduino.h>

uint8_t readByte(uint8_t pin)
{
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
        value |= (digitalRead(pin) << (7 - i));
        delayMicroseconds(BIT_DELAY);
    }
    return value;
}

uint16_t readUInt16(uint8_t pin)
{
    uint8_t low = readByte(pin);
    uint8_t high = readByte(pin);
    return (high << 8) | low;
}

void sendByte(uint8_t pin, uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        digitalWrite(pin, (byte >> i) & 1);
        delayMicroseconds(BIT_DELAY);
    }
    digitalWrite(pin, LOW);
}

void sendUInt16(uint8_t pin, uint16_t val)
{
    sendByte(pin, val & 0xFF);
    sendByte(pin, val >> 8);
}
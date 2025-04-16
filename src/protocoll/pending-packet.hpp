#pragma once

#include <Arduino.h>
#include "./logical.hpp"

struct PendingPacket
{
    Pocket pocket;
    uint8_t sendPin;
    uint8_t attempts;
    unsigned long lastSendTime; // in milliseconds

    PendingPacket(const Pocket &p, uint8_t pin, uint8_t att, unsigned long time)
        : pocket(p), sendPin(pin), attempts(att), lastSendTime(time) {}
};
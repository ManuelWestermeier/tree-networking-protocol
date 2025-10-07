#pragma once

#include "./physikal.hpp"

void PhysikalNode::sendNormalPocket(Pocket &p, uint8_t pin)
{
    Serial.print("[Protocol] sendNormalPocket: sending on pin ");
    Serial.println(pin);

    pinMode(pin, OUTPUT);
    // start signal
    digitalWrite(pin, HIGH);
    delayMicroseconds(BIT_DELAY);
    // data frame
    digitalWrite(pin, HIGH);
    delayMicroseconds(BIT_DELAY);

    for (auto a : p.address)
    {
        sendUInt16(pin, a);
    }
    sendUInt16(pin, 0); // End of address marker

    for (int i = 0; i < DATASIZE; i++)
    {
        sendByte(pin, p.data[i]);
    }

    sendUInt16(pin, p.id);

    sendUInt16(pin, p.checksum);

    pinMode(pin, INPUT); // Switch back to receive mode

    Serial.print("[Protocol] sendNormalPocket: queued packet with checksum ");
    Serial.println(p.checksum, HEX);
}

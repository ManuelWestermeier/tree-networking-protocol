#pragma once
#include <cstring>

struct Pocket
{
    Address address;
    char data[11];
    uint16_t checksum;

    Pocket(Address a, const char *d) : address(a)
    {
        // Fill with spaces first
        memset(data, ' ', 10);

        // Copy up to 10 characters from d
        size_t len = strlen(d);
        size_t copyLen = len > 10 ? 10 : len;
        memcpy(data, d, copyLen);

        // Null-terminate only if there's room (optional depending on expected use)
        data[10] = '\0';

        checksum = calculateChecksum();
    }

    uint16_t calculateChecksum() const
    {
        uint16_t sum1 = 0;
        uint16_t sum2 = 0;

        // Add address parts
        for (uint16_t part : address)
        {
            sum1 = (sum1 + part) % 255;
            sum2 = (sum2 + sum1) % 255;
        }

        // Add data bytes
        for (int i = 0; i < 10; i++)
        {
            sum1 = (sum1 + static_cast<uint8_t>(data[i])) % 255;
            sum2 = (sum2 + sum1) % 255;
        }

        return ((sum2 << 8) | sum1) ^ address.size(); // Combine sums into one 16-bit checksum xor with the adress length
    }
};

#include "CircularBuffer/ImageBuffer.h"
#include <iostream>
#include <vector>
#include <cstdint>

int main()
{
    // Create a circular buffer with 10 elements, each of size 4 bytes
    ImageBuffer buffer(10, 4);

    // Fill the buffer with some data
    for (int i = 0; i < 15; i++)
    {
        uint8_t data[4] = {static_cast<uint8_t>(i), static_cast<uint8_t>(i + 1), static_cast<uint8_t>(i + 2), static_cast<uint8_t>(i + 3)};
        buffer.push(data);
    }

    return 0;
}
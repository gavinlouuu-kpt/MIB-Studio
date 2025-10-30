#pragma once

#include <cstddef>
#include <cstdint>

namespace mib {

enum class PixelFormat {
    BGR8,
    RGB8,
    MONO8
};

struct Frame {
    uint8_t* data;
    std::size_t sizeBytes;
    int width;
    int height;
    PixelFormat format;
    std::uint64_t timestampNs;
};

} // namespace mib



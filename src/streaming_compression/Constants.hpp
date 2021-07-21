#ifndef STREAMING_COMPRESSION_CONSTANTS_HPP
#define STREAMING_COMPRESSION_CONSTANTS_HPP

// C++ libraries
#include <cstddef>
#include <cstdint>

namespace streaming_compression {
    enum class CompressorType : uint8_t {
        ZSTD = 0x10,
        Passthrough = 0xFF,
    };
}

#endif //STREAMING_COMPRESSION_CONSTANTS_HPP
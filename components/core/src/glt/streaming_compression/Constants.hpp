#ifndef GLT_STREAMING_COMPRESSION_CONSTANTS_HPP
#define GLT_STREAMING_COMPRESSION_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>

namespace glt::streaming_compression {
enum class CompressorType : uint8_t {
    ZSTD = 0x10,
    Passthrough = 0xFF,
};
}  // namespace glt::streaming_compression

#endif  // GLT_STREAMING_COMPRESSION_CONSTANTS_HPP

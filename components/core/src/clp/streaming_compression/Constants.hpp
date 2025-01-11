#ifndef CLP_STREAMING_COMPRESSION_CONSTANTS_HPP
#define CLP_STREAMING_COMPRESSION_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>

namespace clp::streaming_compression {
enum class CompressorType : uint8_t {
    ZSTD = 0x10,
    LZMA = 0x20,
    Passthrough = 0xFF,
};
}  // namespace clp::streaming_compression

#endif  // CLP_STREAMING_COMPRESSION_CONSTANTS_HPP

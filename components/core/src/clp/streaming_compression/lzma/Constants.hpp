#ifndef STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP
#define STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP

#include <lzma.h>

// C++ libraries
#include <cstddef>
#include <cstdint>

namespace clp::streaming_compression::lzma {
constexpr int cDefaultCompressionLevel{3};
constexpr uint32_t cDefaultDictionarySize{LZMA_DICT_SIZE_DEFAULT};
}  // namespace streaming_compression::lzma

#endif  // STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP

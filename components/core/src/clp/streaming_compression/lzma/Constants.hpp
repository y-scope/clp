#ifndef STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP
#define STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP

#include <cstdint>

#include <lzma.h>

namespace clp::streaming_compression::lzma {
constexpr int cDefaultCompressionLevel{3};
constexpr int cMinCompressionLevel{0};
constexpr int cMaxCompressionLevel{9};
constexpr uint32_t cDefaultDictionarySize{LZMA_DICT_SIZE_DEFAULT};
}  // namespace clp::streaming_compression::lzma

#endif  // STREAMING_COMPRESSION_LZMA_CONSTANTS_HPP

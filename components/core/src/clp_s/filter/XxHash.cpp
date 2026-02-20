#include "XxHash.hpp"

#define XXH_INLINE_ALL
#include <xxhash.h>

namespace clp_s::filter::xxhash {
uint64_t hash64(std::string_view value, uint64_t seed) {
    return static_cast<uint64_t>(XXH3_64bits_withSeed(value.data(), value.size(), seed));
}
}  // namespace clp_s::filter::xxhash

#include "XxHash.hpp"

#include <cstdint>
#include <string_view>

#include <xxhash.h>

namespace clp_s::filter::xxhash {
auto hash64(std::string_view value, uint64_t seed) -> uint64_t {
    return static_cast<uint64_t>(XXH3_64bits_withSeed(value.data(), value.size(), seed));
}
}  // namespace clp_s::filter::xxhash

#ifndef CLP_S_FILTER_XX_HASH_HPP
#define CLP_S_FILTER_XX_HASH_HPP

#include <cstdint>
#include <string_view>

namespace clp_s::filter::xxhash {
/**
 * @param value
 * @param seed
 * @return The 64-bit seeded XXH3 hash of `value`.
 */
[[nodiscard]] auto hash64(std::string_view value, uint64_t seed) -> uint64_t;
}  // namespace clp_s::filter::xxhash

#endif  // CLP_S_FILTER_XX_HASH_HPP

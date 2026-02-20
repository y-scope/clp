#ifndef CLP_S_FILTER_XX_HASH_HPP
#define CLP_S_FILTER_XX_HASH_HPP

#include <cstdint>
#include <string_view>

namespace clp_s::filter::xxhash {
[[nodiscard]] uint64_t hash64(std::string_view value, uint64_t seed);
}  // namespace clp_s::filter::xxhash

#endif  // CLP_S_FILTER_XX_HASH_HPP

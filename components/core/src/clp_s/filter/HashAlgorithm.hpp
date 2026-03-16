#ifndef CLP_S_FILTER_HASH_ALGORITHM_HPP
#define CLP_S_FILTER_HASH_ALGORITHM_HPP

#include <cstdint>
#include <optional>
#include <string_view>

namespace clp_s::filter {
/**
 * Hash algorithms supported by the filter serialization format.
 */
enum class HashAlgorithm : uint8_t {
    Xxh364 = 0x00,
};

/**
 * @param hash_algorithm Encoded algorithm ID from serialized payload.
 * @return Parsed hash algorithm when supported; std::nullopt otherwise.
 */
[[nodiscard]] auto try_parse_hash_algorithm(uint8_t hash_algorithm) -> std::optional<HashAlgorithm>;

/**
 * @param hash_algorithm Hash algorithm to apply.
 * @param value
 * @param seed
 * @return 64-bit hash result produced by `hash_algorithm`.
 */
[[nodiscard]] auto hash64(HashAlgorithm hash_algorithm, std::string_view value, uint64_t seed)
        -> uint64_t;
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_HASH_ALGORITHM_HPP

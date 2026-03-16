#include "HashAlgorithm.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

#include "XxHash.hpp"

namespace clp_s::filter {
auto try_parse_hash_algorithm(uint8_t hash_algorithm) -> std::optional<HashAlgorithm> {
    switch (hash_algorithm) {
        case static_cast<uint8_t>(HashAlgorithm::Xxh364):
            return HashAlgorithm::Xxh364;
        default:
            return std::nullopt;
    }
}

auto hash64(HashAlgorithm hash_algorithm, std::string_view value, uint64_t seed) -> uint64_t {
    switch (hash_algorithm) {
        case HashAlgorithm::Xxh364:
            return xxhash::hash64(value, seed);
        default:
            break;
    }

    return xxhash::hash64(value, seed);
}
}  // namespace clp_s::filter

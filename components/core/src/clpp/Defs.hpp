#ifndef CLPP_CONSTANTS_HPP
#define CLPP_CONSTANTS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace clpp {
using log_shape_id_t = uint64_t;

/**
 * Matches the value of log-surgeon's `Match::encoding_idx` which is assigned in insertion order.
 * uint16_t is used to match log-surgeon's type.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class EncodingType : uint16_t {
    None = 0,
    Float = 1,
    Int = 2,
};

struct EncodingPattern {
    EncodingType type;
    std::string_view name;
    std::string_view pattern;
};

inline constexpr std::array cEncodingPatterns{
        EncodingPattern{
                .type = EncodingType::Float,
                .name = "float",
                .pattern = R"(-?\d+\.\d+([eE]-?\d+)?)"
        },
        EncodingPattern{.type = EncodingType::Int, .name = "int", .pattern = R"(-?\d+)"},
};

// Verify cEncodingPatterns index i corresponds to EncodingType value i+1 so that
// static_cast<EncodingType>(match->encoding_idx) is always correct.
template <size_t... is>
[[nodiscard]] constexpr auto verify_encoding_order(std::index_sequence<is...>) -> bool {
    return ((static_cast<uint16_t>(std::get<is>(cEncodingPatterns).type) == is + 1) && ...);
}

static_assert(verify_encoding_order(std::make_index_sequence<cEncodingPatterns.size()>{}));

inline constexpr std::string_view cShapeFunction{"shape"};
inline constexpr std::string_view cDecomposeFunction{"decompose"};
}  // namespace clpp

#endif  // CLPP_CONSTANTS_HPP

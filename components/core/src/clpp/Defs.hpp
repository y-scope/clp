#ifndef CLPP_CONSTANTS_HPP
#define CLPP_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace clpp {
// Types
using log_shape_id_t = uint64_t;

struct EncodingPattern {
    std::string_view name;
    std::string_view pattern;
};

// Static constants
inline constexpr std::string_view cFloatEncodingName{"float"};
inline constexpr std::string_view cIntEncodingName{"int"};

/*
 * A match's encodings appear in insertion order at runtime.
 */
inline constexpr std::array cEncodingPatterns{
        EncodingPattern{.name = cFloatEncodingName, .pattern = R"(-?\d+\.\d+)"},
        EncodingPattern{.name = cIntEncodingName, .pattern = R"(-?\d+)"}
};

inline constexpr std::string_view cDecomposedSuffix{"@decomposed"};
inline constexpr std::string_view cShapeSuffix{"@shape"};
}  // namespace clpp

#endif  // CLPP_CONSTANTS_HPP

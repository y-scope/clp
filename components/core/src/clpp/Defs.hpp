#ifndef CLPP_CONSTANTS_HPP
#define CLPP_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace clpp {
using log_shape_id_t = uint64_t;

struct EncodingPattern {
    std::string_view name;
    std::string_view pattern;
};

inline constexpr std::string_view cFloatEncodingName{"float"};
inline constexpr std::string_view cIntEncodingName{"int"};
// A log_surgeon::Match's possible encodings are listed in insertion order to the parsing spec.
inline constexpr std::array cEncodingPatterns{
        EncodingPattern{.name = cFloatEncodingName, .pattern = R"(-?\d+\.\d+([eE]-?\d+)?)"},
        EncodingPattern{.name = cIntEncodingName, .pattern = R"(-?\d+)"}
};

inline constexpr std::string_view cShapeFunction{"shape"};
inline constexpr std::string_view cDecomposeFunction{"decompose"};
}  // namespace clpp

#endif  // CLPP_CONSTANTS_HPP

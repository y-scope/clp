#ifndef CLPP_CONSTANTS_HPP
#define CLPP_CONSTANTS_HPP

#include <cstdint>
#include <string_view>

namespace clpp {
using log_shape_id_t = uint64_t;

inline constexpr std::string_view cDecomposedSuffix{"@decomposed"};
inline constexpr std::string_view cShapeSuffix{"@shape"};
}  // namespace clpp

#endif  // CLPP_CONSTANTS_HPP

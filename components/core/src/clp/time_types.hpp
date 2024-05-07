#ifndef CLP_TIME_TYPES_HPP
#define CLP_TIME_TYPES_HPP

#include <chrono>

namespace clp {
// We use seconds resolution to support GPS clock offsets
using UtcOffset = std::chrono::seconds;
}  // namespace clp

#endif  // CLP_TIME_TYPES_HPP

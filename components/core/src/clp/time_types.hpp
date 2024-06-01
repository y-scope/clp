#ifndef CLP_TIME_TYPES_HPP
#define CLP_TIME_TYPES_HPP

#include <chrono>

namespace clp {
// Although the highest resolution for UTC Offsets that we know of is seconds for GPS clock offsets,
// we use milliseconds both for simplicity when working with our default timestamp resolution and
// for future-proofing.
using UtcOffset = std::chrono::milliseconds;
}  // namespace clp

#endif  // CLP_TIME_TYPES_HPP

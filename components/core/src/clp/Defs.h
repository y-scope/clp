#ifndef CLP_DEFS_H
#define CLP_DEFS_H

#include <atomic>
#include <cstdint>
#include <limits>

namespace clp {
// Types
using epochtime_t = int64_t;
constexpr epochtime_t cEpochTimeMin = std::numeric_limits<epochtime_t>::min();
constexpr epochtime_t cEpochTimeMax = std::numeric_limits<epochtime_t>::max();

using variable_dictionary_id_t = uint64_t;
constexpr variable_dictionary_id_t cVariableDictionaryIdMax
        = std::numeric_limits<variable_dictionary_id_t>::max();

using logtype_dictionary_id_t = int64_t;
constexpr logtype_dictionary_id_t cLogtypeDictionaryIdMax
        = std::numeric_limits<logtype_dictionary_id_t>::max();

using archive_format_version_t = uint32_t;

using segment_id_t = uint64_t;
constexpr segment_id_t cInvalidSegmentId = std::numeric_limits<segment_id_t>::max();

using encoded_variable_t = int64_t;

using group_id_t = uint64_t;

// Constants
constexpr char cDefaultConfigFilename[] = ".clp.rc";
}  // namespace clp

#endif  // CLP_DEFS_H

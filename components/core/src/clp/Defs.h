#ifndef CLP_DEFS_H
#define CLP_DEFS_H

#include <atomic>
#include <cstdint>
#include <limits>

namespace clp {
// Types
typedef int64_t epochtime_t;
constexpr epochtime_t cEpochTimeMin = std::numeric_limits<epochtime_t>::min();
constexpr epochtime_t cEpochTimeMax = std::numeric_limits<epochtime_t>::max();

typedef uint64_t variable_dictionary_id_t;
constexpr variable_dictionary_id_t cVariableDictionaryIdMax
        = std::numeric_limits<variable_dictionary_id_t>::max();

typedef int64_t logtype_dictionary_id_t;
constexpr logtype_dictionary_id_t cLogtypeDictionaryIdMax
        = std::numeric_limits<logtype_dictionary_id_t>::max();

typedef uint16_t archive_format_version_t;
// This flag is used to maintain two separate streams of archive format
// versions:
// - Development versions (which can change frequently as necessary) which
//   should have the flag
// - Production versions (which should be changed with care and as infrequently
//   as possible) which should not have the flag
constexpr archive_format_version_t cArchiveFormatDevVersionFlag = 0x8000;

typedef uint64_t segment_id_t;
constexpr segment_id_t cInvalidSegmentId = std::numeric_limits<segment_id_t>::max();

typedef int64_t encoded_variable_t;

typedef uint64_t group_id_t;

// Constants
constexpr char cDefaultConfigFilename[] = ".clp.rc";
}  // namespace clp

#endif  // CLP_DEFS_H

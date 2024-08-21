#ifndef GLT_DEFS_H
#define GLT_DEFS_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace glt {
// Types
using epochtime_t = int64_t;
constexpr epochtime_t cEpochTimeMin = std::numeric_limits<epochtime_t>::min();
constexpr epochtime_t cEpochTimeMax = std::numeric_limits<epochtime_t>::max();
#define SECONDS_TO_EPOCHTIME(x) x * 1000
#define MICROSECONDS_TO_EPOCHTIME(x) 0

using variable_dictionary_id_t = uint64_t;
constexpr variable_dictionary_id_t cVariableDictionaryIdMax
        = std::numeric_limits<variable_dictionary_id_t>::max();

using logtype_dictionary_id_t = int64_t;
constexpr logtype_dictionary_id_t cLogtypeDictionaryIdMax
        = std::numeric_limits<logtype_dictionary_id_t>::max();

using archive_format_version_t = uint16_t;
// This flag is used to maintain two separate streams of archive format
// versions:
// - Development versions (which can change frequently as necessary) which
//   should have the flag
// - Production versions (which should be changed with care and as infrequently
//   as possible) which should not have the flag
constexpr archive_format_version_t cArchiveFormatDevVersionFlag = 0x8000;

using file_id_t = uint32_t;
using segment_id_t = uint64_t;
constexpr segment_id_t cInvalidSegmentId = std::numeric_limits<segment_id_t>::max();

using offset_t = size_t;
using encoded_variable_t = int64_t;
using combined_table_id_t = uint64_t;

using group_id_t = uint64_t;

using pipeline_id_t = uint64_t;
constexpr pipeline_id_t cPipelineIdMax = std::numeric_limits<pipeline_id_t>::max();
using atomic_pipeline_id_t = std::atomic_uint64_t;

// Macros
// Rounds up VALUE to be a multiple of MULTIPLE
#define ROUND_UP_TO_MULTIPLE(VALUE, MULTIPLE) ((VALUE + MULTIPLE - 1) / MULTIPLE) * MULTIPLE

// Constants
constexpr char cDefaultConfigFilename[] = ".clp.rc";
constexpr int cMongoDbDuplicateKeyErrorCode = 11'000;
}  // namespace glt

#endif  // GLT_DEFS_H

// Code from CLP

#ifndef CLP_S_DEFS_HPP
#define CLP_S_DEFS_HPP

// C++ libraries
#include <atomic>
#include <cinttypes>
#include <cstdint>
#include <limits>

namespace clp_s {
// Types
typedef int64_t epochtime_t;
static epochtime_t const cEpochTimeMin = INT64_MIN;
static epochtime_t const cEpochTimeMax = INT64_MAX;
static double const cDoubleEpochTimeMin = std::numeric_limits<double>::lowest();
static double const cDoubleEpochTimeMax = std::numeric_limits<double>::max();
#define SECONDS_TO_EPOCHTIME(x) x * 1000
#define MICROSECONDS_TO_EPOCHTIME(x) 0
#define EPOCHTIME_T_PRINTF_FMT PRId64

typedef uint64_t variable_dictionary_id_t;
static variable_dictionary_id_t const cVariableDictionaryIdMax = UINT64_MAX;
typedef int64_t logtype_dictionary_id_t;
static logtype_dictionary_id_t const cLogtypeDictionaryIdMax = INT64_MAX;

typedef uint16_t archive_format_version_t;
// This flag is used to maintain two separate streams of archive format versions:
// - Development versions (which can change frequently as necessary) which should have the flag
// - Production versions (which should be changed with care and as infrequently as possible)
// which should not have the flag
constexpr archive_format_version_t cArchiveFormatDevelopmentVersionFlag = 0x8000;

typedef uint64_t file_id_t;
typedef uint64_t segment_id_t;
typedef int64_t encoded_variable_t;
}  // namespace clp_s

// Macros
// Relative version of __FILE__
#define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)
// Rounds up VALUE to be a multiple of MULTIPLE
#define ROUND_UP_TO_MULTIPLE(VALUE, MULTIPLE) ((VALUE + MULTIPLE - 1) / MULTIPLE) * MULTIPLE

#endif  // CLP_S_DEFS_HPP

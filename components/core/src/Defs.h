#ifndef DEFS_H
#define DEFS_H

// C++ libraries
#include <atomic>
#include <cstdint>

// Types
typedef int64_t epochtime_t;
static const epochtime_t cEpochTimeMin = INT64_MIN;
static const epochtime_t cEpochTimeMax = INT64_MAX;
#define SECONDS_TO_EPOCHTIME(x) x*1000
#define MICROSECONDS_TO_EPOCHTIME(x) 0

typedef uint64_t variable_dictionary_id_t;
static const variable_dictionary_id_t cVariableDictionaryIdMax = UINT64_MAX;
typedef int64_t logtype_dictionary_id_t;
static const logtype_dictionary_id_t cLogtypeDictionaryIdMax = INT64_MAX;

typedef uint16_t archive_format_version_t;
// This flag is used to maintain two separate streams of archive format versions:
// - Development versions (which can change frequently as necessary) which should have the flag
// - Production versions (which should be changed with care and as infrequently as possible) which should not have the flag
constexpr archive_format_version_t cArchiveFormatDevelopmentVersionFlag = 0x8000;

typedef uint64_t file_id_t;
typedef uint64_t segment_id_t;
constexpr segment_id_t cInvalidSegmentId = UINT64_MAX;

typedef int64_t encoded_variable_t;

typedef uint64_t group_id_t;

typedef uint64_t pipeline_id_t;
constexpr pipeline_id_t cPipelineIdMax = UINT64_MAX;
typedef std::atomic_uint64_t atomic_pipeline_id_t;

enum LogVerbosity : uint8_t {
    LogVerbosity_FATAL = 0,
    LogVerbosity_ERROR,
    LogVerbosity_WARN,
    LogVerbosity_INFO,
    LogVerbosity_DEBUG,
    LogVerbosity_TRACE,
    LogVerbosity_UNKNOWN,
    LogVerbosity_Length
};

// Macros
// Relative version of __FILE__
#define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)
// Rounds up VALUE to be a multiple of MULTIPLE
#define ROUND_UP_TO_MULTIPLE(VALUE, MULTIPLE) ((VALUE + MULTIPLE - 1) / MULTIPLE) * MULTIPLE

// Constants
constexpr char cDefaultConfigFilename[] = ".clp.rc";
constexpr int cMongoDbDuplicateKeyErrorCode = 11000;

/**
 * Gets the underlying type of the given enum
 * @tparam T
 * @param enum_member
 * @return The underlying type of the given enum
 */
template <typename T>
constexpr typename std::underlying_type<T>::type enum_to_underlying_type(T enum_member) {
    return static_cast<typename std::underlying_type<T>::type>(enum_member);
}

#endif // DEFS_H

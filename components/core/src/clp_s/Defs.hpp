#ifndef CLP_S_DEFS_HPP
#define CLP_S_DEFS_HPP

#include <cstdint>
#include <limits>

namespace clp_s {
// Types
using epochtime_t = int64_t;
constexpr epochtime_t cEpochTimeMin{std::numeric_limits<epochtime_t>::min()};
constexpr epochtime_t cEpochTimeMax{std::numeric_limits<epochtime_t>::max()};
constexpr double cDoubleEpochTimeMin{std::numeric_limits<double>::lowest()};
constexpr double cDoubleEpochTimeMax{std::numeric_limits<double>::max()};

using variable_dictionary_id_t = uint64_t;
constexpr variable_dictionary_id_t cVariableDictionaryIdMax{
        std::numeric_limits<variable_dictionary_id_t>::max()
};
using logtype_dictionary_id_t = int64_t;
constexpr logtype_dictionary_id_t cLogtypeDictionaryIdMax{
        std::numeric_limits<logtype_dictionary_id_t>::max()
};

using archive_format_version_t = uint16_t;
// This flag is used to maintain two separate streams of archive format versions:
// - Development versions (which can change frequently as necessary) which should have the flag
// - Production versions (which should be changed with care and as infrequently as possible)
// which should not have the flag
constexpr archive_format_version_t cArchiveFormatDevelopmentVersionFlag{0x8000};

using file_id_t = uint64_t;
using segment_id_t = uint64_t;
using encoded_variable_t = int64_t;
}  // namespace clp_s
#endif  // CLP_S_DEFS_HPP

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

using logtype_id_t = uint64_t;

using mpt_node_id_t = int32_t;

/**
 * The flavour of CLP used to compress an archive. The numeric value of each flavour is stored in
 * the archive and must never change. I.e. elements in this enum can never be reordered and new
 * flavours must be added to the end of the enum before `Unknown`.
 */
enum class ClpFlavour : uint8_t {
    Clps = 0,
    Clpp = 1,
    Unknown = 2
};
}  // namespace clp_s
#endif  // CLP_S_DEFS_HPP

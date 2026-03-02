#ifndef CLP_S_ARCHIVESTATS_HPP
#define CLP_S_ARCHIVESTATS_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/Defs.h>
#include <clp/streaming_archive/Constants.hpp>
#include <clp_s/Array.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>

namespace clp_s {
/*
 * Tracks the stats for a log type in an archive.
 */
class LogTypeStat {
public:
    // Methods
    [[nodiscard]] auto compress(ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<LogTypeStat>;

    [[nodiscard]] auto get_count() const -> size_t { return m_count; }

    auto increment_count() -> void { ++m_count; }

private:
    // Data members
    size_t m_count{};
};

using LogTypeStats = Array<LogTypeStat, logtype_id_t>;

/*
 * Tracks the stats for a dictionary variables in an archive.
 */
class VariableStat {
public:
    // Constructors
    VariableStat() = default;

    VariableStat(std::string_view type) : m_type(type) {}

    // Methods
    [[nodiscard]] auto compress(ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<VariableStat>;

    [[nodiscard]] auto get_count() const -> size_t { return m_count; }

    [[nodiscard]] auto get_type() const -> std::string_view { return m_type; }

    auto increment_count() -> void { ++m_count; }

private:
    // Data members
    std::string m_type;
    size_t m_count{};
};

using VariableStats = Array<VariableStat, clp::variable_dictionary_id_t>;

struct ExperimentalStats {
    LogTypeStats m_logtype_stats;
    VariableStats m_var_stats;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEWRITER_HPP

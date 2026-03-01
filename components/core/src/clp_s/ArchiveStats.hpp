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
    LogTypeStat() = default;

    LogTypeStat(std::vector<std::string> const& type_names)
            : m_var_type_names(type_names.begin(), type_names.end()) {}

    [[nodiscard]] auto compress(ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<LogTypeStat>;

    [[nodiscard]] auto get_count() const -> size_t { return m_count; }

    auto increment_count() -> void { ++m_count; }

    [[nodiscard]] auto get_var_type_names() const -> std::vector<std::string> const& {
        return m_var_type_names;
    }

private:
    size_t m_count{};
    std::vector<std::string> m_var_type_names;
};

using LogTypeStats = Array<LogTypeStat, clp::logtype_dictionary_id_t>;

/*
 * Tracks the stats for a dictionary variables in an archive.
 */
class VariableStat {
public:
    VariableStat() = default;

    VariableStat(std::string_view type) : m_type(type) {}

    [[nodiscard]] auto compress(ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<VariableStat>;

    [[nodiscard]] auto get_count() const -> size_t { return m_count; }

    [[nodiscard]] auto get_type() const -> std::string_view { return m_type; }

    auto increment_count() -> void { ++m_count; }

private:
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

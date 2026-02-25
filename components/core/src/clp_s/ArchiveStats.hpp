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
class ArchiveStats {
public:
    // Constructors
    explicit ArchiveStats(
            std::string id,
            epochtime_t begin_timestamp,
            epochtime_t end_timestamp,
            size_t uncompressed_size,
            size_t compressed_size,
            nlohmann::json range_index,
            bool is_split
    )
            : m_id{std::move(id)},
              m_begin_timestamp{begin_timestamp},
              m_end_timestamp{end_timestamp},
              m_uncompressed_size{uncompressed_size},
              m_compressed_size{compressed_size},
              m_range_index(std::move(range_index)),  // Avoid {} to prevent wrapping in JSON array.
              m_is_split{is_split} {}

    // Methods
    /**
     * @return The contents of `ArchiveStats` as a JSON object in a string.
     */
    [[nodiscard]] auto as_string() const -> std::string {
        namespace archive = clp::streaming_archive::cMetadataDB::Archive;
        namespace file = clp::streaming_archive::cMetadataDB::File;
        constexpr std::string_view cRangeIndex{"range_index"};

        nlohmann::json const json_msg
                = {{archive::Id, m_id},
                   {archive::BeginTimestamp, m_begin_timestamp},
                   {archive::EndTimestamp, m_end_timestamp},
                   {archive::UncompressedSize, m_uncompressed_size},
                   {archive::Size, m_compressed_size},
                   {file::IsSplit, m_is_split},
                   {cRangeIndex, m_range_index}};
        return json_msg.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
    }

    [[nodiscard]] auto get_id() const -> std::string const& { return m_id; }

    [[nodiscard]] auto get_begin_timestamp() const -> epochtime_t { return m_begin_timestamp; }

    [[nodiscard]] auto get_end_timestamp() const -> epochtime_t { return m_end_timestamp; }

    [[nodiscard]] auto get_uncompressed_size() const -> size_t { return m_uncompressed_size; }

    [[nodiscard]] auto get_compressed_size() const -> size_t { return m_compressed_size; }

    [[nodiscard]] auto get_range_index() const -> nlohmann::json const& { return m_range_index; }

    [[nodiscard]] auto get_is_split() const -> bool { return m_is_split; }

private:
    std::string m_id;
    epochtime_t m_begin_timestamp{};
    epochtime_t m_end_timestamp{};
    size_t m_uncompressed_size{};
    size_t m_compressed_size{};
    nlohmann::json m_range_index;
    bool m_is_split{};
};

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

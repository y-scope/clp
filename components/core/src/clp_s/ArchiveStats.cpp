#include "ArchiveStats.hpp"

#include <cstddef>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/Defs.h>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>

namespace clp_s {
auto ArchiveStats::LogTypeStats::record(clp::logtype_dictionary_id_t id) -> void {
    if (m_stats.size() < id + 1) {
        m_stats.resize(id + 1);
    }
    m_stats.at(id).m_count++;
}

auto ArchiveStats::LogTypeStats::compress(ZstdCompressor& compressor)
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_stats.size());
    for (auto const& stat : m_stats) {
        compressor.write_numeric_value(stat.m_count);
    }
    return ystdlib::error_handling::success();
}

auto ArchiveStats::LogTypeStats::decompress(ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    size_t num_stats{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(num_stats)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    m_stats.resize(num_stats);
    for (size_t i{0}; i < num_stats; ++i) {
        if (ErrorCodeSuccess != decompressor.try_read_numeric_value(m_stats.at(i).m_count)) {
            return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
        }
    }
    return ystdlib::error_handling::success();
}

auto ArchiveStats::VariableStats::record(clp::variable_dictionary_id_t id, std::string_view type)
        -> void {
    if (m_stats.size() < id + 1) {
        m_stats.resize(id + 1);
        m_stats.at(id).m_type = type;
    }
    m_stats.at(id).m_count++;
}

auto ArchiveStats::VariableStats::compress(ZstdCompressor& compressor)
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_stats.size());
    for (auto const& stat : m_stats) {
        compressor.write_numeric_value(stat.m_count);
        compressor.write_numeric_value(stat.m_type.size());
        compressor.write(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<char const*>(stat.m_type.data()),
                stat.m_type.size()
        );
    }
    return ystdlib::error_handling::success();
}

auto ArchiveStats::VariableStats::decompress(ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    size_t num_stats{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(num_stats)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    m_stats.resize(num_stats);
    for (auto& stat : m_stats) {
        if (ErrorCodeSuccess != decompressor.try_read_numeric_value(stat.m_count)) {
            return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
        }
        size_t type_size{};
        if (ErrorCodeSuccess != decompressor.try_read_numeric_value(type_size)) {
            return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
        }
        stat.m_type.resize(type_size);
        if (ErrorCodeSuccess
            != decompressor.try_read_exact_length(
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    reinterpret_cast<char*>(stat.m_type.data()),
                    stat.m_type.size()
            ))
        {
            return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
        }
    }
    return ystdlib::error_handling::success();
}
}  // namespace clp_s

#include "RangeIndexWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "ErrorCode.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
auto RangeIndexWriter::open_range(size_t start_index) -> ErrorCode {
    if (false == m_ranges.empty()) {
        auto const& range = m_ranges.back();
        if (false == range.end_index.has_value()) {
            return ErrorCodeNotReady;
        }
        if (start_index < range.start_index || start_index < range.end_index.value()) {
            return ErrorCodeBadParam;
        }
    }
    m_ranges.emplace_back(start_index);
    return ErrorCodeSuccess;
}

auto RangeIndexWriter::close_range(size_t end_index) -> ErrorCode {
    if (m_ranges.empty()) {
        return ErrorCodeNotReady;
    }
    auto& range = m_ranges.back();
    if (range.end_index.has_value()) {
        return ErrorCodeNotReady;
    }
    if (end_index < range.start_index) {
        return ErrorCodeBadParam;
    }
    range.end_index = end_index;
    return ErrorCodeSuccess;
}

auto RangeIndexWriter::write(ZstdCompressor& writer, nlohmann::json& metadata) -> ErrorCode {
    if (m_ranges.empty()) {
        metadata = nlohmann::json::array();
        return ErrorCodeSuccess;
    }

    nlohmann::json ranges_array;
    for (auto const& range : m_ranges) {
        if (false == range.end_index.has_value()) {
            return ErrorCodeNotReady;
        }

        nlohmann::json obj;
        obj[cStartIndexName] = range.start_index;
        obj[cEndIndexName] = range.end_index.value();

        nlohmann::json fields_obj;
        for (auto const& [key, value] : range.fields) {
            fields_obj[key] = value;
        }
        obj[cMetadataFieldsName] = std::move(fields_obj);

        ranges_array.emplace_back(std::move(obj));
    }
    metadata = ranges_array;

    std::vector<char> serialized_range_index;
    nlohmann::json::to_msgpack(ranges_array, serialized_range_index);
    try {
        writer.write_numeric_value(ArchiveMetadataPacketType::RangeIndex);
        writer.write_numeric_value<uint32_t>(serialized_range_index.size());
        writer.write(serialized_range_index.data(), serialized_range_index.size());
    } catch (std::exception const& e) {
        return ErrorCodeFailure;
    }
    m_ranges.clear();

    return ErrorCodeSuccess;
}
}  // namespace clp_s

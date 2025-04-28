#include "RangeIndexWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <outcome/outcome.hpp>

#include "ErrorCode.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
auto RangeIndexWriter::open_range(size_t start_index)
        -> OUTCOME_V2_NAMESPACE::std_result<handle_t> {
    for (auto const& range : m_ranges) {
        if (start_index < range.start_index || false == range.end_index.has_value()
            || start_index < range.end_index.value())
        {
            return std::errc::invalid_argument;
        }
    }
    handle_t handle{m_ranges.size()};
    m_ranges.emplace_back(start_index);
    return handle;
}

auto RangeIndexWriter::close_range(handle_t handle, size_t end_index) -> ErrorCode {
    if (handle >= m_ranges.size()) {
        return ErrorCodeOutOfBounds;
    }

    auto& range = m_ranges.at(handle);
    if (range.end_index.has_value()) {
        return ErrorCodeNotReady;
    }

    if (end_index < range.start_index) {
        return ErrorCodeBadParam;
    }

    range.end_index = end_index;
    return ErrorCodeSuccess;
}

auto RangeIndexWriter::write(ZstdCompressor& writer) -> ErrorCode {
    if (m_ranges.empty()) {
        return ErrorCodeSuccess;
    }

    nlohmann::json ranges_array;
    for (auto const& range : m_ranges) {
        if (false == range.end_index.has_value()) {
            return ErrorCodeNotReady;
        }

        nlohmann::json obj;
        obj["s"] = range.start_index;
        obj["e"] = range.end_index.value();

        nlohmann::json fields_obj;
        for (auto const& [key, value] : range.fields) {
            fields_obj[key] = value;
        }
        obj["f"] = std::move(fields_obj);

        ranges_array.emplace_back(std::move(obj));
    }

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

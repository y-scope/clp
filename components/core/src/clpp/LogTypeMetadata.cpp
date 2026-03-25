#include "LogTypeMetadata.hpp"

#include <cstddef>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto LogTypeMetadata::compress(clp_s::ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_parent_matches.size());
    for (auto const& match : m_parent_matches) {
        compressor.write_numeric_value(match.m_rule_id);
        compressor.write_numeric_value(match.m_capture_id);
        compressor.write_numeric_value(match.m_parent_id);
        compressor.write_numeric_value(match.m_start);
        compressor.write_numeric_value(match.m_size);
    }
    return ystdlib::error_handling::success();
}

auto LogTypeMetadata::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<LogTypeMetadata> {
    LogTypeMetadata metadata;
    size_t matches_size{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(matches_size)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    metadata.m_parent_matches.reserve(matches_size);

    for (size_t i{0}; i < matches_size; ++i) {
        uint16_t rule_id{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(rule_id)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        uint32_t capture_id{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(capture_id)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        uint32_t parent_id{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(parent_id)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        size_t start{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(start)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        size_t size{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        metadata.emplace_parent_match(rule_id, capture_id, parent_id, start, size);
    }
    return metadata;
}
}  // namespace clpp

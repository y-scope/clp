#include "FilterReader.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>

#include "BloomFilter.hpp"
#include "ErrorCode.hpp"
#include "FilterOptions.hpp"

namespace clp_s::filter {
namespace {
[[nodiscard]] auto try_parse_filter_type_id(uint8_t filter_type_id) -> std::optional<FilterType> {
    if (static_cast<uint8_t>(FilterType::Bloom) == filter_type_id) {
        return FilterType::Bloom;
    }

    return std::nullopt;
}

[[nodiscard]] auto try_parse_filter_normalization_id(uint8_t filter_normalization_id)
        -> std::optional<FilterNormalization> {
    if (static_cast<uint8_t>(FilterNormalization::None) == filter_normalization_id) {
        return FilterNormalization::None;
    }
    if (static_cast<uint8_t>(FilterNormalization::Lowercase) == filter_normalization_id) {
        return FilterNormalization::Lowercase;
    }

    return std::nullopt;
}
}  // namespace

auto FilterReader::try_read(clp::ReaderInterface& reader)
        -> ystdlib::error_handling::Result<FilterReader> {
    std::array<char, cFilterFileMagic.size()> magic{};
    if (clp::ErrorCode_Success
        != reader.try_read_exact_length(magic.data(), cFilterFileMagic.size()))
    {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }
    if (0 != std::memcmp(magic.data(), cFilterFileMagic.data(), cFilterFileMagic.size())) {
        return ErrorCode{ErrorCodeEnum::CorruptFilterFile};
    }

    uint8_t filter_type_id{};
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(filter_type_id)) {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }

    auto const optional_filter_type{try_parse_filter_type_id(filter_type_id)};
    if (false == optional_filter_type.has_value()) {
        return ErrorCode{ErrorCodeEnum::UnsupportedFilterType};
    }

    uint8_t filter_normalization_id{};
    if (clp::ErrorCode_Success != reader.try_read_numeric_value(filter_normalization_id)) {
        return ErrorCode{ErrorCodeEnum::ReadFailure};
    }

    auto const optional_filter_normalization{
            try_parse_filter_normalization_id(filter_normalization_id)
    };
    if (false == optional_filter_normalization.has_value()) {
        return ErrorCode{ErrorCodeEnum::UnsupportedFilterNormalization};
    }

    auto bloom_filter{YSTDLIB_ERROR_HANDLING_TRYX(BloomFilter::try_read(reader))};
    return FilterReader{
            optional_filter_type.value(),
            optional_filter_normalization.value(),
            std::move(bloom_filter)
    };
}

auto FilterReader::possibly_contains(std::string_view value) const -> bool {
    auto const normalized_value{normalize_string(value, m_normalization)};
    return m_bloom_filter.possibly_contains(normalized_value);
}

auto FilterReader::possibly_contains_wildcard(std::string_view) const -> bool {
    switch (m_type) {
        case FilterType::Bloom:
            return true;
    }

    return true;
}

FilterReader::FilterReader(
        FilterType type,
        FilterNormalization normalization,
        BloomFilter bloom_filter
)
        : m_type{type},
          m_normalization{normalization},
          m_bloom_filter{std::move(bloom_filter)} {}
}  // namespace clp_s::filter

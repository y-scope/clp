#include "FilterBuilder.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/WriterInterface.hpp>

#include "BloomFilter.hpp"
#include "ErrorCode.hpp"
#include "FilterOptions.hpp"

namespace clp_s::filter {
auto FilterBuilder::create(
        FilterType type,
        FilterNormalization normalization,
        size_t expected_num_elements,
        double false_positive_rate
) -> ystdlib::error_handling::Result<FilterBuilder> {
    if (FilterNormalization::None != normalization
        && FilterNormalization::Lowercase != normalization)
    {
        return ErrorCode{ErrorCodeEnum::UnsupportedFilterNormalization};
    }

    switch (type) {
        case FilterType::Bloom: {
            auto bloom_filter{YSTDLIB_ERROR_HANDLING_TRYX(
                    BloomFilter::create(expected_num_elements, false_positive_rate)
            )};
            return FilterBuilder{type, normalization, std::move(bloom_filter)};
        }
    }

    return ErrorCode{ErrorCodeEnum::UnsupportedFilterType};
}

auto FilterBuilder::add(std::string_view value) -> void {
    auto const normalized_value{normalize_string(value, m_normalization)};
    m_bloom_filter.add(normalized_value);
}

auto FilterBuilder::write_to_file(clp::WriterInterface& writer) const -> void {
    writer.write(cFilterFileMagic.data(), cFilterFileMagic.size());
    writer.write_numeric_value(static_cast<uint8_t>(m_type));
    writer.write_numeric_value(static_cast<uint8_t>(m_normalization));
    m_bloom_filter.write_to_file(writer);
}

FilterBuilder::FilterBuilder(
        FilterType type,
        FilterNormalization normalization,
        BloomFilter bloom_filter
)
        : m_type{type},
          m_normalization{normalization},
          m_bloom_filter{std::move(bloom_filter)} {}
}  // namespace clp_s::filter

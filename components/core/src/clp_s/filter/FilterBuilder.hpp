#ifndef CLP_S_FILTER_FILTER_BUILDER_HPP
#define CLP_S_FILTER_FILTER_BUILDER_HPP

#include <cstddef>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/WriterInterface.hpp>

#include "BloomFilter.hpp"
#include "FilterOptions.hpp"

namespace clp_s::filter {
/**
 * Builds variable-dictionary filters and writes them to a writer.
 */
class FilterBuilder {
public:
    /**
     * Creates a filter builder from the given parameters.
     * @param type
     * @param normalization
     * @param expected_num_elements Expected number of inserted values.
     * @param false_positive_rate
     * @return A result containing an initialized FilterBuilder on success.
     * @return ErrorCodeEnum::UnsupportedFilterType if `type` is not supported.
     * @return ErrorCodeEnum::UnsupportedFilterNormalization if `normalization` is not supported.
     * @return Forwards the underlying filter implementation's return values on construction
     * failure.
     */
    [[nodiscard]] static auto create(
            FilterType type,
            FilterNormalization normalization,
            size_t expected_num_elements,
            double false_positive_rate
    ) -> ystdlib::error_handling::Result<FilterBuilder>;

    /**
     * Adds a value to the configured filter after applying the configured normalization strategy.
     * @param value
     */
    auto add(std::string_view value) -> void;

    /**
     * Writes a serialized filter representation to a writer.
     * @param writer
     */
    auto write_to_file(clp::WriterInterface& writer) const -> void;

    /**
     * @return The configured filter type.
     */
    [[nodiscard]] auto get_type() const -> FilterType { return m_type; }

    [[nodiscard]] auto get_normalization() const -> FilterNormalization { return m_normalization; }

private:
    FilterBuilder(FilterType type, FilterNormalization normalization, BloomFilter bloom_filter);

    FilterType m_type;
    FilterNormalization m_normalization;
    BloomFilter m_bloom_filter;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_FILTER_BUILDER_HPP

#ifndef CLP_S_FILTER_FILTER_READER_HPP
#define CLP_S_FILTER_FILTER_READER_HPP

#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>

#include "BloomFilter.hpp"
#include "FilterOptions.hpp"

namespace clp_s::filter {
/**
 * Reads serialized filters and performs membership checks.
 */
class FilterReader {
public:
    /**
     * Reads a serialized filter from a reader.
     * @param reader
     * @return A result containing an initialized FilterReader on success, or an error code
     * indicating the failure:
     * - ErrorCodeEnum::CorruptFilterFile for malformed filter file metadata.
     * - ErrorCodeEnum::UnsupportedFilterType if the filter type ID is not supported.
     * - ErrorCodeEnum::UnsupportedFilterNormalization if the filter normalization ID is not
     *   supported.
     * - ErrorCodeEnum::ReadFailure for truncated/failed reads.
     * - Forwards the underlying filter implementation's return values on payload read
     * failure.
     */
    [[nodiscard]] static auto try_read(clp::ReaderInterface& reader)
            -> ystdlib::error_handling::Result<FilterReader>;

    /**
     * Checks whether the filter may contain the value after applying the configured
     * normalization strategy.
     * @param value
     * @return true if the filter may contain `value`; false if it definitely does not.
     */
    [[nodiscard]] auto possibly_contains(std::string_view value) const -> bool;

    /**
     * Checks whether the filter may contain the wildcard search string.
     * The caller is responsible for determining whether `value` should be treated as a wildcard
     * query.
     * @param value
     * @return true if the filter may contain `value`; false if it definitely does not.
     */
    [[nodiscard]] auto possibly_contains_wildcard(std::string_view value) const -> bool;

    [[nodiscard]] auto get_type() const -> FilterType { return m_type; }

    [[nodiscard]] auto get_normalization() const -> FilterNormalization { return m_normalization; }

private:
    FilterReader(FilterType type, FilterNormalization normalization, BloomFilter bloom_filter);

    FilterType m_type{};
    FilterNormalization m_normalization{};
    BloomFilter m_bloom_filter;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_FILTER_READER_HPP

#ifndef CLP_S_FILTER_READER_HPP
#define CLP_S_FILTER_READER_HPP

#include <string_view>

#include "BloomFilter.hpp"
#include "FilterConfig.hpp"

namespace clp {
class ReaderInterface;
}  // namespace clp

namespace clp_s::filter {
class FilterReader {
public:
    /**
     * @return true when a valid filter payload is read, false otherwise.
     */
    [[nodiscard]] bool read_from_file(clp::ReaderInterface& reader);

    /**
     * Returns true if the filter might contain the value.
     * For FilterType::None, always returns true.
     */
    [[nodiscard]] bool possibly_contains(std::string_view value) const;

    [[nodiscard]] FilterType get_type() const { return m_type; }

private:
    FilterType m_type{FilterType::None};
    BloomFilter m_filter{};
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_READER_HPP

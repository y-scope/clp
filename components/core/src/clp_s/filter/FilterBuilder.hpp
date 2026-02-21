#ifndef CLP_S_FILTER_BUILDER_HPP
#define CLP_S_FILTER_BUILDER_HPP

#include <cstddef>
#include <string>
#include <string_view>

#include "BloomFilter.hpp"
#include "FilterConfig.hpp"

namespace clp_s::filter {
class FilterBuilder {
public:
    FilterBuilder(FilterConfig const& config, size_t num_elements);

    void add(std::string_view value);
    /**
     * @return false when filter building is disabled (type == None); true when a filter is written.
     * @throws Any exception raised by file I/O.
     */
    [[nodiscard]] bool write(std::string const& filter_path) const;

private:
    FilterConfig m_config;
    BloomFilter m_filter{};
    bool m_enabled{false};
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_BUILDER_HPP

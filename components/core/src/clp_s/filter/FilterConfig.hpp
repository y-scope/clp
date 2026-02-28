#ifndef CLP_S_FILTER_CONFIG_HPP
#define CLP_S_FILTER_CONFIG_HPP

#include <cstdint>
#include <optional>
#include <string_view>

namespace clp_s {
/**
 * Supported filter types for variable dictionaries.
 */
enum class FilterType : uint8_t {
    None = 0,
    Bloom = 1,
};

constexpr double kDefaultFilterFalsePositiveRate{0.01};

struct FilterConfig {
    FilterType type{FilterType::None};
    double false_positive_rate{kDefaultFilterFalsePositiveRate};
};

[[nodiscard]] std::optional<FilterType> parse_filter_type(std::string_view type_str);
[[nodiscard]] std::string_view filter_type_to_string(FilterType type);
}  // namespace clp_s

#endif  // CLP_S_FILTER_CONFIG_HPP

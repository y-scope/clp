#include "FilterOptions.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <clp/string_utils/string_utils.hpp>

namespace clp_s::filter {
auto try_parse_filter_type_id(uint8_t filter_type_id) -> std::optional<FilterType> {
    if (static_cast<uint8_t>(FilterType::Bloom) == filter_type_id) {
        return FilterType::Bloom;
    }

    return std::nullopt;
}

auto try_parse_filter_type(std::string_view filter_type_string) -> std::optional<FilterType> {
    if ("bloom" == filter_type_string) {
        return FilterType::Bloom;
    }

    return std::nullopt;
}

auto try_parse_filter_normalization_id(uint8_t filter_normalization_id)
        -> std::optional<FilterNormalization> {
    if (static_cast<uint8_t>(FilterNormalization::None) == filter_normalization_id) {
        return FilterNormalization::None;
    }
    if (static_cast<uint8_t>(FilterNormalization::Lowercase) == filter_normalization_id) {
        return FilterNormalization::Lowercase;
    }

    return std::nullopt;
}

auto normalize_string(std::string_view value, FilterNormalization normalization) -> std::string {
    std::string normalized{value};
    switch (normalization) {
        case FilterNormalization::None:
            return normalized;
        case FilterNormalization::Lowercase:
            clp::string_utils::to_lower(normalized);
            return normalized;
    }

    return normalized;
}
}  // namespace clp_s::filter

#include "FilterConfig.hpp"

#include <string>

namespace clp_s {
auto parse_filter_type(std::string_view type_str) -> std::optional<FilterType> {
    if (type_str == "none") {
        return FilterType::None;
    }
    if (type_str == "bloom") {
        return FilterType::Bloom;
    }
    return std::nullopt;
}

auto filter_type_to_string(FilterType type) -> std::string_view {
    switch (type) {
        case FilterType::None:
            return "none";
        case FilterType::Bloom:
            return "bloom";
    }
    return "unknown";
}
}  // namespace clp_s

#include "FilterConfig.hpp"

#include <array>

namespace clp_s {
namespace {
constexpr std::array<std::pair<std::string_view, FilterType>, 2> kFilterTypes{{
        {"none", FilterType::None},
        {"bloom", FilterType::Bloom},
}};
}  // namespace

std::optional<FilterType> parse_filter_type(std::string_view type_str) {
    for (auto const& [name, type] : kFilterTypes) {
        if (type_str == name) {
            return type;
        }
    }
    return std::nullopt;
}

std::string_view filter_type_to_string(FilterType type) {
    for (auto const& [name, mapped_type] : kFilterTypes) {
        if (type == mapped_type) {
            return name;
        }
    }
    return "unknown";
}
}  // namespace clp_s

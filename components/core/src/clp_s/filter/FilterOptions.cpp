#include "FilterOptions.hpp"

#include <optional>
#include <string>
#include <string_view>

#include <clp/string_utils/string_utils.hpp>

namespace clp_s::filter {
namespace {
constexpr std::string_view cBloomFilterTypeString{"bloom"};
}  // namespace

auto try_parse_filter_type(std::string_view filter_type_string) -> std::optional<FilterType> {
    if (cBloomFilterTypeString == filter_type_string) {
        return FilterType::Bloom;
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

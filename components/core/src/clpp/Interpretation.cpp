#include "Interpretation.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clpp/ErrorCode.hpp>
#include <clpp/TextShape.hpp>

namespace clpp {
namespace {
constexpr bool cDecompositionSupported{CLP_BUILD_CLPP_DECOMPOSITION};
}  // namespace

auto supports_decomposition() noexcept -> bool {
    return cDecompositionSupported;
}

auto decompose_by_rule_name(
        log_surgeon::ParsingSpec& parsing_spec,
        std::string_view query,
        std::string_view rule_name
) -> ystdlib::error_handling::Result<std::vector<Interpretation>> {
    if constexpr (false == cDecompositionSupported) {
        return ClppErrorCode{ClppErrorCodeEnum::UnsupportedDecomposition};
    }
    auto const sub_query_sets{parsing_spec.search_by_name(query, rule_name)};
    std::vector<Interpretation> interpretations;
    interpretations.reserve(sub_query_sets.size());
    for (auto const& sub_queries : sub_query_sets) {
        TextShape<std::string> shape_query;
        std::vector<LeafQuery> leaf_queries;
        for (auto const& sub_query : sub_queries) {
            if (sub_query.name.empty()) {
                shape_query.escape_and_append(sub_query.value);
            } else {
                leaf_queries.emplace_back(sub_query.name, sub_query.value, leaf_queries.size());
                shape_query.append_placeholder(sub_query.name);
            }
        }
        interpretations.emplace_back(std::move(shape_query), std::move(leaf_queries));
    }
    return interpretations;
}

auto decompose_by_log_shapes(
        log_surgeon::ParsingSpec& parsing_spec,
        std::string_view query,
        std::vector<log_surgeon::CCharArray> const& log_shapes
) -> ystdlib::error_handling::Result<std::vector<std::vector<std::vector<LeafQuery>>>> {
    if constexpr (false == cDecompositionSupported) {
        return ClppErrorCode{ClppErrorCodeEnum::UnsupportedDecomposition};
    }

    auto const ls_interpretations_per_shape{parsing_spec.search_by_log_shapes(query, log_shapes)};
    std::vector<std::vector<std::vector<LeafQuery>>> interpretations_per_shape;
    interpretations_per_shape.reserve(ls_interpretations_per_shape.size());
    for (auto const& sub_query_per_interpretation : ls_interpretations_per_shape) {
        std::vector<std::vector<LeafQuery>> leaf_queries_per_interpretation;
        leaf_queries_per_interpretation.reserve(sub_query_per_interpretation.size());
        for (auto const& sub_queries : sub_query_per_interpretation) {
            std::vector<LeafQuery> leaf_queries;
            for (auto const& sub_query : sub_queries) {
                if (false == sub_query.name.empty()) {
                    leaf_queries.emplace_back(sub_query.name, sub_query.value, leaf_queries.size());
                }
            }
            leaf_queries_per_interpretation.emplace_back(std::move(leaf_queries));
        }
        interpretations_per_shape.push_back(std::move(leaf_queries_per_interpretation));
    }
    return interpretations_per_shape;
}

auto split_qualified_name(std::string_view const qualified_name) -> std::vector<std::string_view> {
    std::vector<std::string_view> rule_names;
    size_t start{0};
    while (true) {
        auto end{qualified_name.find('.', start)};
        if (std::string::npos == end) {
            rule_names.emplace_back(qualified_name.substr(start));
            break;
        }
        rule_names.emplace_back(qualified_name.substr(start, end - start));
        start = end + 1;
    }
    return rule_names;
}
}  // namespace clpp

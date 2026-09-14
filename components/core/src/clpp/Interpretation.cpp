#include "Interpretation.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#if CLP_BUILD_CLPP_DECOMPOSITION
    #include <system_error>

    #include <clpp/ErrorCode.hpp>
    #include <ystdlib/error_handling/Result.hpp>
#endif

#include <log_surgeon/log_surgeon.hpp>

namespace clpp {
#if CLP_BUILD_CLPP_DECOMPOSITION
namespace {
/**
 * Builds a query interpretation from log-surgeon sub-query segments. Segments without a rule name
 * append static text or a wildcard to the shape query. Segments with a rule name contain a leaf
 * query and append a placeholder to the shape query.
 */
auto build_interpretation(std::vector<log_surgeon::SubQuery> const& sub_queries) -> Interpretation;

auto build_interpretation(std::vector<log_surgeon::SubQuery> const& sub_queries) -> Interpretation {
    TextShape<std::string> shape_query;
    std::vector<LeafQuery> leaf_queries;
    for (auto const& sub_query : sub_queries) {
        if (sub_query.qualified_name.empty()) {
            shape_query.escape_and_append(sub_query.value);
        } else {
            leaf_queries.emplace_back(sub_query.qualified_name, sub_query.value);
            shape_query.append_placeholder(sub_query.qualified_name);
        }
    }
    return {std::move(shape_query), std::move(leaf_queries)};
}
}  // namespace

auto decompose_by_rule_name(
        log_surgeon::Parser& parser,
        std::string_view query,
        std::string_view rule_name
) -> std::vector<Interpretation> {
    auto const sub_query_sets{parser.search_by_name(query, rule_name)};
    std::vector<Interpretation> interpretations;
    interpretations.reserve(sub_query_sets.size());
    for (auto const& sub_queries : sub_query_sets) {
        interpretations.emplace_back(build_interpretation(sub_queries));
    }
    return interpretations;
}

// Leaf rule match segments surrounded by wildcard segments (e.g. `*(?<rule>*match)*`) must be
// expanded to match any leaf match of that name in the log shape.
auto decompose_by_log_shapes(
        log_surgeon::Parser& parser,
        std::string_view query,
        std::span<std::string_view const> log_shapes
) -> std::vector<std::vector<Interpretation>> {
    std::vector<log_surgeon::CCharArray> ffi_shapes;
    ffi_shapes.reserve(log_shapes.size());
    for (auto const shape : log_shapes) {
        ffi_shapes.push_back(log_surgeon::CCharArray::from_string_view(shape));
    }

    auto const sub_query_sets_per_shape{parser.search_by_log_shapes(query, ffi_shapes)};
    std::vector<std::vector<Interpretation>> interpretations_per_shape;
    interpretations_per_shape.reserve(sub_query_sets_per_shape.size());
    for (auto const& sub_query_set : sub_query_sets_per_shape) {
        std::vector<Interpretation> interpretations;
        interpretations.reserve(sub_query_set.size());
        for (auto const& sub_queries : sub_query_set) {
            interpretations.emplace_back(build_interpretation(sub_queries));
        }
        interpretations_per_shape.push_back(std::move(interpretations));
    }
    return interpretations_per_shape;
}
#else
namespace {
constexpr std::string_view cDecompositionUnsupportedMessage{
        "clp+ query decomposition is not supported in this build; rebuild with"
        " -DCLP_BUILD_CLPP_DECOMPOSITION=ON"
};
}  // namespace

auto decompose_by_rule_name(log_surgeon::Parser&, std::string_view, std::string_view)
        -> std::vector<Interpretation> {
    throw std::system_error{
            ystdlib::error_handling::make_error_code(
                    clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Unsupported}
            ),
            std::string{cDecompositionUnsupportedMessage}
    };
}

auto
decompose_by_log_shapes(log_surgeon::Parser&, std::string_view, std::span<std::string_view const>)
        -> std::vector<std::vector<Interpretation>> {
    throw std::system_error{
            ystdlib::error_handling::make_error_code(
                    clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Unsupported}
            ),
            std::string{cDecompositionUnsupportedMessage}
    };
}
#endif

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

#include "DecomposedQuery.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clpp/ErrorCode.hpp>
#include <clpp/LogShapeUtils.hpp>

namespace clpp {
auto DecomposedQuery::decompose_query(
        log_surgeon::ParserHandle& parser,
        std::string_view rule_name,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    auto const interpretations{parser.query_interpretations(
            log_surgeon::CCharArray::from_string_view(rule_name),
            log_surgeon::CCharArray::from_string_view(query)
    )};

    if (interpretations.empty()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::DecomposeQueryFailure};
    }

    std::vector<Interpretation> interps;
    for (auto const& sub_queries : interpretations) {
        std::string shape_query;
        std::vector<LeafQuery> leaf_queries;
        for (auto const& sub_query : sub_queries) {
            if (sub_query.qualified_name.empty()) {
                shape_query.append(clpp::escape_shape_text(sub_query.value));
            } else {
                leaf_queries.emplace_back(sub_query.qualified_name, sub_query.value);
                shape_query.append(fmt::format("%{}%", sub_query.qualified_name));
            }
        }
        interps.emplace_back(shape_query, leaf_queries);
    }

    return DecomposedQuery{interps};
}

auto DecomposedQuery::split_qualified_name(std::string_view const qualified_name)
        -> std::vector<std::string_view> {
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

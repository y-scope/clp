#include "DecomposedQuery.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <fmt/format.h>
#include <log_surgeon/generated_bindings.hpp>
#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clpp/ErrorCode.hpp>

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
                shape_query.append(sub_query.value);
            } else {
                leaf_queries.emplace_back(sub_query.qualified_name, sub_query.value);
                shape_query.append(fmt::format("%{}%", sub_query.qualified_name));
            }
        }
        interps.emplace_back(shape_query, leaf_queries);
    }

    return DecomposedQuery{interps};
}

auto DecomposedQuery::create_parent_match_dicts(log_surgeon::EventHandle const& event) -> std::
        pair<absl::flat_hash_map<uint32_t, log_surgeon::Match const>,
             absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Match const>> {
    absl::flat_hash_map<uint32_t, log_surgeon::Match const> root_matches;
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Match const> parent_matches;
    for (auto const& match : event.get_all_matches()) {
        if (match.is_leaf) {
            continue;
        }
        if (0 == match.sub_rule_id) {
            root_matches.emplace(match.rule_idx, match);
        } else {
            parent_matches.emplace(std::pair{match.rule_idx, match.sub_rule_id}, match);
        }
    }
    return {root_matches, parent_matches};
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

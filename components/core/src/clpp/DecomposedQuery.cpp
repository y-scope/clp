#include "DecomposedQuery.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
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
        std::optional<std::string_view> rule_name,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    auto const interpretations{parser.query_interpretations(
            log_surgeon::CCharArray::from_string_view(rule_name.value()),
            log_surgeon::CCharArray::from_string_view(query)
    )};

    if (interpretations.empty()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::DecomposeQueryFailure};
    }

    DecomposedQuery decomposed_query;
    decomposed_query.m_log_type.reserve(query.size());
    for (auto const& sub_queries : interpretations) {
        for (auto const& sub_query : sub_queries) {
            if (sub_query.qualified_name.empty()) {
                decomposed_query.m_log_type.append(sub_query.value);
            } else {
                decomposed_query.m_leaf_queries.emplace_back(
                        sub_query.qualified_name,
                        sub_query.value
                );
                decomposed_query.m_log_type.append(fmt::format("%{}%", sub_query.qualified_name));
            }
        }
    }

    return decomposed_query;
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

auto DecomposedQuery::get_qualified_name(log_surgeon::Match const& match) -> std::string {
    std::vector<std::string_view> names;
    auto const* cur{&match};
    while (0 != cur->sub_rule_id) {
        names.emplace_back(cur->ffi_pointers.sub_rule_name.as_cpp_view());
        cur = cur->ffi_pointers.parent;
        if (nullptr == cur) {
            break;
        }
    }
    if (nullptr != cur) {
        names.emplace_back(cur->ffi_pointers.rule_name.as_cpp_view());
    }

    std::string qualified_name;
    for (auto it{names.rbegin()}; names.rend() != it; ++it) {
        if (names.rbegin() != it) {
            qualified_name.append(".");
        }
        qualified_name.append(*it);
    }
    return qualified_name;
}
}  // namespace clpp

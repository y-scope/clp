#include "DecomposedQuery.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <fmt/format.h>
#include <log_surgeon/generated_bindings.hpp>
#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>
// #include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
namespace {
auto get_full_type_name(
        log_surgeon::Capture const& cap,
        absl::flat_hash_map<uint32_t, log_surgeon::Capture const> rules,
        absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Capture const>
                parent_captures
) -> std::vector<std::string>;

auto get_type_name(log_surgeon::Capture const& cap) -> std::string;

auto get_type_name(log_surgeon::Capture const& cap) -> std::string {
    if (0 == cap.capture_id) {
        return std::string{cap.ffi_pointers.variable_name.as_cpp_view()};
    }
    return std::string{cap.ffi_pointers.capture_name.as_cpp_view()};
}

auto get_full_type_name(
        log_surgeon::Capture const& cap,
        absl::flat_hash_map<uint32_t, log_surgeon::Capture const> rules,
        absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Capture const>
                parent_captures
) -> std::vector<std::string> {
    std::vector<std::string> types;
    auto const* cur{&cap};
    while (0 != cur->capture_id) {
        types.emplace_back(cap.ffi_pointers.capture_name.as_cpp_view());
        if (0 == cur->parent_id) {
            cur = &rules.at(cur->rule_idx.index);
        } else {
            cur = &parent_captures.at({cur->rule_idx.index, cur->parent_id});
        }
    }
    types.emplace_back(cur->ffi_pointers.variable_name.as_cpp_view());
    return types;
}
}  // namespace

auto DecomposedQuery::decompose_query(
        log_surgeon::ParserHandle& parser,
        std::optional<std::string_view> rule_type,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    size_t parser_pos{0};
    auto event{parser.next_event(query, &parser_pos)};
    if (false == event.has_value() || query.size() != parser_pos) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    auto [rules, parent_captures]{create_parent_dicts(event.value())};
    if (rule_type.has_value()
        && (1 != rules.size()
            || rule_type != rules.begin()->second.ffi_pointers.variable_name.as_cpp_view()))
    {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    DecomposedQuery decomposed_query;
    decomposed_query.m_log_type.reserve(query.size());
    size_t query_pos{0};
    for (size_t i{0};; ++i) {
        auto const cap{event->get_leaf_capture(i)};
        if (false == cap.has_value()) {
            break;
        }

        decomposed_query.m_leaves.emplace_back(
                get_full_type_name(*cap, rules, parent_captures),
                cap->ffi_pointers.lexeme.as_cpp_view()
        );

        decomposed_query.m_log_type.append(query.substr(query_pos, cap->range.start - query_pos));
        decomposed_query.m_log_type.append(fmt::format("%{}%", get_type_name(*cap)));
        query_pos = cap->range.end;
    }
    decomposed_query.m_log_type.append(query.substr(query_pos));
    return decomposed_query;
}

// TODO clpp: need a way to get type names for parents
// atm this only works when the parent type only contains leaves (no nesting)
auto DecomposedQuery::decompose_query(
        log_surgeon::Schema const* schema,
        std::string_view type,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    auto const* search_result{log_surgeon::log_surgeon_search_by_named_type(
            schema,
            log_surgeon::CCharArray::from_string_view(type),
            log_surgeon::CCharArray::from_string_view(query)
    )};
    if (nullptr == search_result) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    DecomposedQuery decomposed_query{search_result};
    std::string log_type{};
    log_type.reserve(query.size());
    size_t query_pos{0};
    size_t leaf_caps_len{0};
    auto const* leaf_caps{
            log_surgeon::log_surgeon_search_result_get_leaf_captures(search_result, &leaf_caps_len)
    };
    for (size_t i{0}; i < leaf_caps_len; ++i) {
        auto const cap{leaf_caps[i]};
        decomposed_query.m_leaves.emplace_back(
                // get_full_type_name(*cap, rules, parent_captures),
                std::vector{get_type_name(cap)},
                cap.ffi_pointers.lexeme.as_cpp_view()
        );

        log_type.append(query.substr(query_pos, cap.range.start - query_pos));
        log_type.append(fmt::format("%{}%", get_type_name(cap)));
        query_pos = cap.range.end;
    }

    log_type.append(query.substr(query_pos));
    decomposed_query.m_log_type = log_type;
    return decomposed_query;
}

auto DecomposedQuery::create_parent_dicts(log_surgeon::EventHandle const& event) -> std::
        pair<absl::flat_hash_map<uint32_t, log_surgeon::Capture const>,
             absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Capture const>> {
    absl::flat_hash_map<uint32_t, log_surgeon::Capture const> rules;
    absl::flat_hash_map<std::pair<uint32_t, uint32_t>, log_surgeon::Capture const> parent_captures;
    for (auto const& cap : event.get_all_captures()) {
        if (cap.is_leaf) {
            continue;
        }
        if (0 == cap.capture_id) {
            rules.emplace(cap.rule_idx.index, cap);
        } else {
            parent_captures.emplace(std::pair{cap.rule_idx.index, cap.capture_id}, cap);
        }
    }
    return {rules, parent_captures};
}
}  // namespace clpp

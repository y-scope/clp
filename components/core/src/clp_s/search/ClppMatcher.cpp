#include "ClppMatcher.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/string_utils/string_utils.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>

#if 0 == CLP_BUILD_CLPP_DECOMPOSITION
    #include <system_error>

    #include <clpp/ErrorCode.hpp>
#endif

namespace clp_s::search {
ClppMatcher::ClppMatcher(ArchiveReader* archive_reader, bool case_sensitive)
        : m_archive_reader{archive_reader},
          m_case_sensitive{case_sensitive} {
    if (false == m_archive_reader->experimental()) {
        return;
    }
    auto const log_shape_dict{m_archive_reader->get_log_shape_dictionary()};
    if (nullptr == log_shape_dict) {
        throw std::runtime_error{"ClppMatcher got a null log shape dictionary"};
    }

    m_schemas_by_log_shape.resize(log_shape_dict->get_entries().size());
    for (auto const& [schema_id, schema] : *m_archive_reader->get_schema_map()) {
        if (auto const log_shape_id{schema.get_view().find_log_shape_id()}) {
            if (*log_shape_id >= m_schemas_by_log_shape.size()) {
                throw std::runtime_error{
                        "ClppMatcher found a schema referencing an unknown log shape ID"
                };
            }
            m_schemas_by_log_shape.at(*log_shape_id).emplace(schema_id);
        }
    }
}

auto ClppMatcher::find_matching_schemas(
        std::string_view rule_name,
        std::optional<std::string_view> shape_query
) const -> std::unordered_set<int32_t> {
    if (shape_query.has_value()) {
        return schemas_for_matching_shapes(rule_name, [&](std::string_view shape) -> bool {
            return clp::string_utils::wildcard_match_unsafe(shape, *shape_query, m_case_sensitive);
        });
    }
    return schemas_for_matching_shapes(rule_name, [](std::string_view) -> bool { return true; });
}

auto ClppMatcher::decompose_query(std::string_view query, std::string_view rule_name)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
#if 0 == CLP_BUILD_CLPP_DECOMPOSITION
    throw std::system_error{
            ystdlib::error_handling::make_error_code(
                    clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Unsupported}
            ),
        "clp+ query decomposition is not supported in this build; rebuild with"
        " -DCLP_BUILD_CLPP_DECOMPOSITION=ON"
    };
#endif
    if (rule_name.empty()) {
        return decompose_by_log_shapes(query);
    }
    return decompose_by_rule_name(query, rule_name);
}

auto ClppMatcher::decompose_by_log_shapes(std::string_view query)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    std::vector<std::string_view> log_shapes;
    auto const& entries{m_archive_reader->get_log_shape_dictionary()->get_entries()};
    log_shapes.reserve(entries.size());
    for (auto const& log_shape : entries) {
        log_shapes.emplace_back(log_shape.get_value());
    }

    if (nullptr == m_parser) {
        m_parser = std::make_unique<log_surgeon::Parser>(log_surgeon::ParsingSpecBuilder{
                YSTDLIB_ERROR_HANDLING_TRYX(m_archive_reader->read_parsing_spec())
        }
                                                                 .build());
    }
    auto interpretations_by_shape{clpp::decompose_by_log_shapes(*m_parser, query, log_shapes)};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations_by_shape.size());
    for (clpp::log_shape_id_t log_shape_id{0}; log_shape_id < interpretations_by_shape.size();
         ++log_shape_id)
    {
        for (auto& interpretation : interpretations_by_shape.at(log_shape_id)) {
            matches.push_back(
                    {.schema_ids{m_schemas_by_log_shape.at(log_shape_id)},
                     .interpretation{std::move(interpretation)}}
            );
        }
    }
    return matches;
}

auto ClppMatcher::decompose_by_rule_name(std::string_view query, std::string_view rule_name)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    if (nullptr == m_parser) {
        m_parser = std::make_unique<log_surgeon::Parser>(log_surgeon::ParsingSpecBuilder{
                YSTDLIB_ERROR_HANDLING_TRYX(m_archive_reader->read_parsing_spec())
        }
                                                                 .build());
    }
    auto interpretations{clpp::decompose_by_rule_name(*m_parser, query, rule_name)};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations.size());
    for (auto& interpretation : interpretations) {
        auto schema_ids{schemas_for_matching_shapes(rule_name, [&](std::string_view shape) -> bool {
            return clp::string_utils::wildcard_match_unsafe(
                    shape,
                    interpretation.m_shape_query.view(),
                    m_case_sensitive
            );
        })};
        if (schema_ids.empty()) {
            continue;
        }
        matches.push_back(
                {.schema_ids{std::move(schema_ids)}, .interpretation{std::move(interpretation)}}
        );
    }
    return matches;
}

auto ClppMatcher::get_shapes(clpp::log_shape_id_t log_shape_id, std::string_view rule_name) const
        -> std::vector<std::string_view> {
    std::string_view const log_shape{
            m_archive_reader->get_log_shape_dictionary()->get_entries().at(log_shape_id).get_value()
    };
    if (rule_name.empty()) {
        return {log_shape};
    }
    std::vector<std::string_view> shapes;
    for (auto const& parent_match :
         m_archive_reader->get_parent_rule_shapes().at(log_shape_id).get())
    {
        if (rule_name == parent_match.m_name) {
            shapes.emplace_back(log_shape.substr(parent_match.m_start, parent_match.m_size));
        }
    }
    return shapes;
}
}  // namespace clp_s::search

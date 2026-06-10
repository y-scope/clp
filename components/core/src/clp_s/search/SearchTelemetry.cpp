#include "SearchTelemetry.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/nostd/string_view.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_metadata.h>
#include <opentelemetry/trace/tracer.h>  // IWYU pragma: keep
#include <xxhash.h>

#include <clp_s/Defs.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/FilterExpr.hpp>
#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ast/OrExpr.hpp>

using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::OrExpr;
using opentelemetry::trace::StatusCode;

namespace clp_s::search {
namespace {
constexpr std::string_view cTracerName{"clp_s.search"};
constexpr std::string_view cSearchArchiveSpanName{"clp_s.search.archive"};

constexpr std::array<std::string_view, 4> cTruthyEnvValues{"1", "true", "yes", "y"};

constexpr std::string_view cAttrSuccess{"clp.search.success"};
constexpr std::string_view cAttrError{"clp.search.error"};
constexpr std::string_view cAttrQueryHash{"clp.search.query_hash"};
constexpr std::string_view cAttrQueryId{"clp.search.query_id"};
constexpr std::string_view cAttrTaskId{"clp.search.task_id"};
constexpr std::string_view cQueryShapeAttrPrefix{"clp.query_shape."};
constexpr std::string_view cPreprocessedQueryShapeAttrPrefix{"clp.query_shape.preprocessed."};
constexpr std::string_view cAttrSuffixColumnTypesPureWildcard{"column_types.pure_wildcard"};
constexpr std::string_view cAttrSuffixColumnTypesSomeWildcard{"column_types.some_wildcard"};
constexpr std::string_view cAttrSuffixColumnTypesNoWildcard{"column_types.no_wildcard"};
constexpr std::string_view cAttrSuffixPredicateTypesString{"predicate_types.string"};
constexpr std::string_view cAttrSuffixPredicateTypesStringWithWildcard{
        "predicate_types.string_with_wildcard"
};
constexpr std::string_view cAttrSuffixPredicateTypesInt{"predicate_types.int"};
constexpr std::string_view cAttrSuffixPredicateTypesFloat{"predicate_types.float"};
constexpr std::string_view cAttrSuffixPredicateTypesNull{"predicate_types.null"};
constexpr std::string_view cAttrSuffixPredicateTypesExactMatch{"predicate_types.exact_match"};
constexpr std::string_view cAttrSuffixPredicateTypesRange{"predicate_types.range"};
constexpr std::string_view cAttrSuffixPredicateTypesExists{"predicate_types.exists"};
constexpr std::string_view cAttrSuffixNumPredicates{"num_predicates"};
constexpr std::string_view cAttrSuffixContainsOrClause{"contains_or_clause"};
constexpr std::string_view cAttrTimeRangeMillis{"clp.query_shape.time_range_millis"};
constexpr std::string_view cAttrTotalArchiveRecords{"clp.search.total_archive_records"};
constexpr std::string_view cAttrCandidateRecordsAfterSchemaMatching{
        "clp.search.candidate_records_after_schema_matching"
};
constexpr std::string_view cAttrRecordsMatchingQuery{"clp.search.records_matching_query"};
constexpr std::string_view cAttrNumMatchedSchemas{"clp.search.num_matched_schemas"};
constexpr std::string_view cAttrNumSchemasWithMatches{"clp.search.num_schemas_with_matches"};
constexpr std::string_view cAttrTerminationStage{"clp.search.termination_stage"};

/**
 * @param sv
 * @return `sv` as an OpenTelemetry `nostd::string_view`.
 */
[[nodiscard]] auto to_nostd_string_view(std::string_view sv) -> opentelemetry::nostd::string_view {
    return opentelemetry::nostd::string_view{sv.data(), sv.size()};
}

/**
 * @param value
 * @return `value` clamped to the maximum `int64_t`, since OpenTelemetry span attributes are signed.
 */
[[nodiscard]] auto to_int64_attribute(uint64_t value) -> int64_t {
    constexpr auto cMaxInt64{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    return value > cMaxInt64 ? std::numeric_limits<int64_t>::max() : static_cast<int64_t>(value);
}

/**
 * @param name
 * @return Whether the given environment variable is set to a truthy value (one of
 * `cTruthyEnvValues`).
 */
[[nodiscard]] auto is_env_var_enabled(char const* name) -> bool {
    // The environment is only read during single-threaded search setup.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    char const* const raw{std::getenv(name)};
    if (nullptr == raw) {
        return false;
    }
    std::string value{raw};
    std::ranges::transform(value, value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return std::ranges::find(cTruthyEnvValues, value) != cTruthyEnvValues.end();
}

/**
 * Increments the column-shape counter in `metrics` corresponding to `column`'s wildcard usage.
 * @param metrics
 * @param column
 */
auto add_column_shape(QueryShapeMetrics& metrics, ColumnDescriptor const& column) -> void {
    if (column.is_pure_wildcard()) {
        ++metrics.column_shape_metrics.pure_wildcard;
    } else if (column.is_unresolved_descriptor()) {
        ++metrics.column_shape_metrics.some_wildcard;
    } else {
        ++metrics.column_shape_metrics.no_wildcard;
    }
}

/**
 * Increments the predicate-type counters in `metrics` for `filter`'s operation and operand type.
 * @param metrics
 * @param filter
 */
auto add_predicate_type(QueryShapeMetrics& metrics, FilterExpr const& filter) -> void {
    auto const op{filter.get_operation()};
    switch (op) {
        case FilterOperation::EXISTS:
        case FilterOperation::NEXISTS:
            ++metrics.predicate_type_metrics.exists;
            return;
        case FilterOperation::EQ:
        case FilterOperation::NEQ:
            ++metrics.predicate_type_metrics.exact_match;
            break;
        case FilterOperation::LT:
        case FilterOperation::GT:
        case FilterOperation::LTE:
        case FilterOperation::GTE:
            ++metrics.predicate_type_metrics.range;
            break;
    }

    auto const operand{filter.get_operand()};
    if (nullptr == operand) {
        return;
    }

    std::string string_value;
    int64_t int_value{};
    double float_value{};
    bool bool_value{};
    if (operand->as_clp_string(string_value, op) || operand->as_var_string(string_value, op)) {
        if (string_value.find('*') == std::string::npos) {
            ++metrics.predicate_type_metrics.string;
        } else {
            ++metrics.predicate_type_metrics.string_with_wildcard;
        }
    }
    if (operand->as_int(int_value, op) || operand->as_timestamp()) {
        ++metrics.predicate_type_metrics.integer;
    }
    if (operand->as_float(float_value, op)) {
        ++metrics.predicate_type_metrics.floating_point;
    }
    if (operand->as_null(op)) {
        ++metrics.predicate_type_metrics.null;
    }
    static_cast<void>(operand->as_bool(bool_value, op));
}

/**
 * Walks `expr` and its descendants, accumulating query-shape metrics (column shapes, predicate
 * types, predicate count, and whether an OR clause is present) into `metrics`.
 * @param metrics
 * @param expr
 */
auto
collect_query_shape_metrics(QueryShapeMetrics& metrics, std::shared_ptr<Expression> const& expr)
        -> void {
    std::vector<std::shared_ptr<Expression>> to_visit;
    if (nullptr != expr) {
        to_visit.push_back(expr);
    }
    while (false == to_visit.empty()) {
        auto const node{to_visit.back()};
        to_visit.pop_back();
        if (nullptr != std::dynamic_pointer_cast<OrExpr>(node)) {
            metrics.contains_or_clause = true;
        }
        if (auto const filter{std::dynamic_pointer_cast<FilterExpr>(node)}; nullptr != filter) {
            ++metrics.num_predicates;
            add_column_shape(metrics, *filter->get_column());
            add_predicate_type(metrics, *filter);
            continue;
        }
        for (auto it{node->op_begin()}; it != node->op_end(); ++it) {
            if (auto const child{std::dynamic_pointer_cast<Expression>(*it)}; nullptr != child) {
                to_visit.push_back(child);
            }
        }
    }
}
}  // namespace

class SearchTelemetrySpan::Impl {
public:
    Impl()
            : m_span{opentelemetry::trace::Provider::GetTracerProvider()
                             ->GetTracer(to_nostd_string_view(cTracerName))
                             ->StartSpan(to_nostd_string_view(cSearchArchiveSpanName))},
              m_scope{std::make_unique<opentelemetry::trace::Scope>(m_span)} {
        m_span->SetAttribute(to_nostd_string_view(cAttrSuccess), true);
    }

    ~Impl() { m_span->End(); }

    Impl(Impl const&) = delete;
    auto operator=(Impl const&) -> Impl& = delete;

    Impl(Impl&&) = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    auto set_query_context(std::string_view query) -> void {
        m_span->SetAttribute(
                to_nostd_string_view(cAttrQueryHash),
                static_cast<int64_t>(XXH3_64bits(query.data(), query.size()))
        );
        // The environment is only read during single-threaded search setup.
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        if (char const* const query_id{std::getenv("CLP_QUERY_ID")}; nullptr != query_id) {
            m_span->SetAttribute(to_nostd_string_view(cAttrQueryId), query_id);
        }
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        if (char const* const task_id{std::getenv("CLP_TASK_ID")}; nullptr != task_id) {
            m_span->SetAttribute(to_nostd_string_view(cAttrTaskId), task_id);
        }
    }

    auto set_query_shape_metrics(QueryShapeMetrics const& metrics) -> void {
        set_query_shape_attributes(cQueryShapeAttrPrefix, metrics);
        if (metrics.time_range_millis.has_value()) {
            m_span->SetAttribute(
                    to_nostd_string_view(cAttrTimeRangeMillis),
                    to_int64_attribute(*metrics.time_range_millis)
            );
        }
    }

    auto set_preprocessed_query_shape_metrics(QueryShapeMetrics const& metrics) -> void {
        set_query_shape_attributes(cPreprocessedQueryShapeAttrPrefix, metrics);
    }

    auto set_search_result_metrics(SearchResultMetrics const& metrics) -> void {
        m_span->SetAttribute(
                to_nostd_string_view(cAttrTotalArchiveRecords),
                to_int64_attribute(metrics.total_archive_records)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrCandidateRecordsAfterSchemaMatching),
                to_int64_attribute(metrics.candidate_records_after_schema_matching)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrRecordsMatchingQuery),
                to_int64_attribute(metrics.records_matching_query)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrNumMatchedSchemas),
                to_int64_attribute(metrics.num_matched_schemas)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrNumSchemasWithMatches),
                to_int64_attribute(metrics.num_schemas_with_matches)
        );
    }

    auto set_termination_stage(std::string_view termination_stage) -> void {
        m_span->SetAttribute(
                to_nostd_string_view(cAttrTerminationStage),
                to_nostd_string_view(termination_stage)
        );
    }

    auto set_error(std::string_view message) -> void {
        m_span->SetAttribute(to_nostd_string_view(cAttrSuccess), false);
        m_span->SetAttribute(to_nostd_string_view(cAttrError), to_nostd_string_view(message));
        m_span->SetStatus(StatusCode::kError, to_nostd_string_view(message));
    }

private:
    /**
     * Records the query-shape counters as attributes whose keys are `key_prefix` followed by the
     * counter's `cAttrSuffix*` constant.
     * @param key_prefix
     * @param metrics
     */
    auto set_query_shape_attributes(std::string_view key_prefix, QueryShapeMetrics const& metrics)
            -> void {
        auto const set_counter = [&](std::string_view key_suffix, uint64_t value) {
            std::string key{key_prefix};
            key += key_suffix;
            m_span->SetAttribute(to_nostd_string_view(key), to_int64_attribute(value));
        };
        set_counter(cAttrSuffixColumnTypesPureWildcard, metrics.column_shape_metrics.pure_wildcard);
        set_counter(cAttrSuffixColumnTypesSomeWildcard, metrics.column_shape_metrics.some_wildcard);
        set_counter(cAttrSuffixColumnTypesNoWildcard, metrics.column_shape_metrics.no_wildcard);
        set_counter(cAttrSuffixPredicateTypesString, metrics.predicate_type_metrics.string);
        set_counter(
                cAttrSuffixPredicateTypesStringWithWildcard,
                metrics.predicate_type_metrics.string_with_wildcard
        );
        set_counter(cAttrSuffixPredicateTypesInt, metrics.predicate_type_metrics.integer);
        set_counter(cAttrSuffixPredicateTypesFloat, metrics.predicate_type_metrics.floating_point);
        set_counter(cAttrSuffixPredicateTypesNull, metrics.predicate_type_metrics.null);
        set_counter(
                cAttrSuffixPredicateTypesExactMatch,
                metrics.predicate_type_metrics.exact_match
        );
        set_counter(cAttrSuffixPredicateTypesRange, metrics.predicate_type_metrics.range);
        set_counter(cAttrSuffixPredicateTypesExists, metrics.predicate_type_metrics.exists);
        set_counter(cAttrSuffixNumPredicates, metrics.num_predicates);

        std::string contains_or_clause_key{key_prefix};
        contains_or_clause_key += cAttrSuffixContainsOrClause;
        m_span->SetAttribute(
                to_nostd_string_view(contains_or_clause_key),
                metrics.contains_or_clause
        );
    }

    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
    std::unique_ptr<opentelemetry::trace::Scope> m_scope;
};

SearchTelemetrySpan::SearchTelemetrySpan() : m_impl{std::make_unique<Impl>()} {}

SearchTelemetrySpan::~SearchTelemetrySpan() = default;

auto SearchTelemetrySpan::set_query_context(std::string_view query) -> void {
    m_impl->set_query_context(query);
}

auto SearchTelemetrySpan::set_query_shape_metrics(QueryShapeMetrics const& metrics) -> void {
    m_impl->set_query_shape_metrics(metrics);
}

auto SearchTelemetrySpan::set_preprocessed_query_shape_metrics(QueryShapeMetrics const& metrics)
        -> void {
    m_impl->set_preprocessed_query_shape_metrics(metrics);
}

auto SearchTelemetrySpan::set_search_result_metrics(SearchResultMetrics const& metrics) -> void {
    m_impl->set_search_result_metrics(metrics);
}

auto SearchTelemetrySpan::set_termination_stage(std::string_view termination_stage) -> void {
    m_impl->set_termination_stage(termination_stage);
}

auto SearchTelemetrySpan::set_error(std::string_view message) -> void {
    m_impl->set_error(message);
}

auto create_query_shape_metrics(
        std::shared_ptr<ast::Expression> const& expr,
        std::optional<epochtime_t> search_begin_ts,
        std::optional<epochtime_t> search_end_ts
) -> QueryShapeMetrics {
    QueryShapeMetrics metrics;
    collect_query_shape_metrics(metrics, expr);
    if (search_begin_ts.has_value() && search_end_ts.has_value()) {
        auto const time_range_millis{*search_end_ts - *search_begin_ts};
        if (0 <= time_range_millis) {
            metrics.time_range_millis = static_cast<uint64_t>(time_range_millis);
        }
    }
    return metrics;
}

auto is_search_telemetry_enabled(bool enable_telemetry) -> bool {
    if (false == enable_telemetry) {
        return false;
    }
    return false == is_env_var_enabled("CLP_DISABLE_TELEMETRY");
}
}  // namespace clp_s::search

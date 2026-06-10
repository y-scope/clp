#include "SearchTelemetry.hpp"

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

constexpr std::string_view cAttrSuccess{"clp.search.success"};
constexpr std::string_view cAttrError{"clp.search.error"};
constexpr std::string_view cAttrQueryHash{"clp.search.query_hash"};
constexpr std::string_view cAttrQueryId{"clp.search.query_id"};
constexpr std::string_view cAttrTaskId{"clp.search.task_id"};
constexpr std::string_view cAttrColumnTypesPureWildcard{
        "clp.query_shape.column_types.pure_wildcard"
};
constexpr std::string_view cAttrColumnTypesSomeWildcard{
        "clp.query_shape.column_types.some_wildcard"
};
constexpr std::string_view cAttrColumnTypesNoWildcard{"clp.query_shape.column_types.no_wildcard"};
constexpr std::string_view cAttrPredicateTypesString{"clp.query_shape.predicate_types.string"};
constexpr std::string_view cAttrPredicateTypesStringWithWildcard{
        "clp.query_shape.predicate_types.string_with_wildcard"
};
constexpr std::string_view cAttrPredicateTypesInt{"clp.query_shape.predicate_types.int"};
constexpr std::string_view cAttrPredicateTypesFloat{"clp.query_shape.predicate_types.float"};
constexpr std::string_view cAttrPredicateTypesNull{"clp.query_shape.predicate_types.null"};
constexpr std::string_view cAttrPredicateTypesExactMatch{
        "clp.query_shape.predicate_types.exact_match"
};
constexpr std::string_view cAttrPredicateTypesRange{"clp.query_shape.predicate_types.range"};
constexpr std::string_view cAttrPredicateTypesExists{"clp.query_shape.predicate_types.exists"};
constexpr std::string_view cAttrNumPredicates{"clp.query_shape.num_predicates"};
constexpr std::string_view cAttrContainsOrClause{"clp.query_shape.contains_or_clause"};
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
 * Increments the column-shape counter in `metrics` corresponding to `column`'s wildcard usage.
 *
 * @param column
 * @param metrics
 */
auto add_column_shape(ColumnDescriptor const& column, QueryShapeMetrics& metrics) -> void {
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
 *
 * @param filter
 * @param metrics
 */
auto add_predicate_type(FilterExpr const& filter, QueryShapeMetrics& metrics) -> void {
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
    if (operand->as_clp_string(string_value, op) || operand->as_var_string(string_value, op)) {
        if (std::string::npos == string_value.find('*')) {
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
}

/**
 * Walks `expr` and its descendants, accumulating query-shape metrics (column shapes, predicate
 * types, predicate count, and whether an OR clause is present) into `metrics`.
 *
 * @param expr
 * @param metrics
 */
auto
collect_query_shape_metrics(std::shared_ptr<Expression> const& expr, QueryShapeMetrics& metrics)
        -> void {
    std::vector<std::shared_ptr<Expression>> to_visit;
    if (nullptr != expr) {
        to_visit.emplace_back(expr);
    }
    while (false == to_visit.empty()) {
        auto const node{to_visit.back()};
        to_visit.pop_back();
        if (nullptr != std::dynamic_pointer_cast<OrExpr>(node)) {
            metrics.contains_or_clause = true;
        }
        if (auto const filter{std::dynamic_pointer_cast<FilterExpr>(node)}; nullptr != filter) {
            ++metrics.num_predicates;
            add_column_shape(*filter->get_column(), metrics);
            add_predicate_type(*filter, metrics);
            continue;
        }
        for (auto it{node->op_begin()}; it != node->op_end(); ++it) {
            if (auto const child{std::dynamic_pointer_cast<Expression>(*it)}; nullptr != child) {
                to_visit.emplace_back(child);
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

    // Delete copy constructor and assignment operator
    Impl(Impl const&) = delete;
    auto operator=(Impl const&) -> Impl& = delete;

    // Delete move constructor and assignment operator
    Impl(Impl&&) = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    // Destructor
    ~Impl() { m_span->End(); }

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
        m_span->SetAttribute(
                to_nostd_string_view(cAttrColumnTypesPureWildcard),
                to_int64_attribute(metrics.column_shape_metrics.pure_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrColumnTypesSomeWildcard),
                to_int64_attribute(metrics.column_shape_metrics.some_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrColumnTypesNoWildcard),
                to_int64_attribute(metrics.column_shape_metrics.no_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesString),
                to_int64_attribute(metrics.predicate_type_metrics.string)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesStringWithWildcard),
                to_int64_attribute(metrics.predicate_type_metrics.string_with_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesInt),
                to_int64_attribute(metrics.predicate_type_metrics.integer)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesFloat),
                to_int64_attribute(metrics.predicate_type_metrics.floating_point)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesNull),
                to_int64_attribute(metrics.predicate_type_metrics.null)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesExactMatch),
                to_int64_attribute(metrics.predicate_type_metrics.exact_match)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesRange),
                to_int64_attribute(metrics.predicate_type_metrics.range)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesExists),
                to_int64_attribute(metrics.predicate_type_metrics.exists)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrNumPredicates),
                to_int64_attribute(metrics.num_predicates)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrContainsOrClause),
                metrics.contains_or_clause
        );
        if (metrics.time_range_millis.has_value()) {
            m_span->SetAttribute(
                    to_nostd_string_view(cAttrTimeRangeMillis),
                    to_int64_attribute(*metrics.time_range_millis)
            );
        }
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
    collect_query_shape_metrics(expr, metrics);
    if (search_begin_ts.has_value() && search_end_ts.has_value()) {
        auto const time_range_millis{*search_end_ts - *search_begin_ts};
        if (0 <= time_range_millis) {
            metrics.time_range_millis = static_cast<uint64_t>(time_range_millis);
        }
    }
    return metrics;
}
}  // namespace clp_s::search

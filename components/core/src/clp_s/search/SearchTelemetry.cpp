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
constexpr std::string_view cTracerName{"clp_s.query"};
constexpr std::string_view cSearchArchiveSpanName{"clp_s.query.archive"};

constexpr std::string_view cAttrSuccess{"clp.query.success"};
constexpr std::string_view cAttrError{"clp.query.error"};
constexpr std::string_view cAttrQueryHash{"clp.query.query_hash"};
constexpr std::string_view cAttrQueryId{"clp.query.query_id"};
constexpr std::string_view cAttrTaskId{"clp.query.task_id"};
constexpr std::string_view cAttrArchiveIdHash{"clp.query.archive_id_hash"};
constexpr std::string_view cAttrColumnTypesPureWildcard{"clp.query.column_types.num_pure_wildcard"};
constexpr std::string_view cAttrColumnTypesSomeWildcard{"clp.query.column_types.num_some_wildcard"};
constexpr std::string_view cAttrColumnTypesNoWildcard{"clp.query.column_types.num_no_wildcard"};
constexpr std::string_view cAttrPredicateTypesString{"clp.query.predicate_types.num_string"};
constexpr std::string_view cAttrPredicateTypesStringWithWildcard{
        "clp.query.predicate_types.num_string_with_wildcard"
};
constexpr std::string_view cAttrPredicateTypesInt{"clp.query.predicate_types.num_int"};
constexpr std::string_view cAttrPredicateTypesFloat{"clp.query.predicate_types.num_float"};
constexpr std::string_view cAttrPredicateTypesNull{"clp.query.predicate_types.num_null"};
constexpr std::string_view cAttrPredicateTypesExactMatch{
        "clp.query.predicate_types.num_exact_match"
};
constexpr std::string_view cAttrPredicateTypesRange{"clp.query.predicate_types.num_range"};
constexpr std::string_view cAttrPredicateTypesExists{"clp.query.predicate_types.num_exists"};
constexpr std::string_view cAttrNumPredicates{"clp.query.num_predicates"};
constexpr std::string_view cAttrContainsOrClause{"clp.query.contains_or_clause"};
constexpr std::string_view cAttrTimeRangeMillis{"clp.query.time_range_millis"};
constexpr std::string_view cAttrTotalArchiveRecords{"clp.query.num_total_archive_records"};
constexpr std::string_view cAttrCandidateRecordsAfterSchemaMatching{
        "clp.query.num_candidate_records_after_schema_matching"
};
constexpr std::string_view cAttrRecordsMatchingQuery{"clp.query.num_records_matching_query"};
constexpr std::string_view cAttrNumMatchedSchemas{"clp.query.num_matched_schemas"};
constexpr std::string_view cAttrNumSchemasWithMatches{"clp.query.num_schemas_with_matches"};
constexpr std::string_view cAttrTerminationStage{"clp.query.termination_stage"};

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
 * @param value
 * @return A non-reversible 64-bit hash of `value` as a signed span attribute.
 */
[[nodiscard]] auto to_hash_attribute(std::string_view value) -> int64_t {
    return static_cast<int64_t>(XXH3_64bits(value.data(), value.size()));
}

/**
 * Increments the column-shape counter in `metrics` corresponding to `column`'s wildcard usage.
 *
 * @param column
 * @param metrics
 */
auto add_column_shape(ColumnDescriptor const& column, QueryShapeMetrics& metrics) -> void {
    if (column.is_pure_wildcard()) {
        ++metrics.column_shape_metrics.num_pure_wildcard;
    } else if (column.is_unresolved_descriptor()) {
        ++metrics.column_shape_metrics.num_some_wildcard;
    } else {
        ++metrics.column_shape_metrics.num_no_wildcard;
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
            ++metrics.predicate_type_metrics.num_exists;
            return;
        case FilterOperation::EQ:
        case FilterOperation::NEQ:
            ++metrics.predicate_type_metrics.num_exact_match;
            break;
        case FilterOperation::LT:
        case FilterOperation::GT:
        case FilterOperation::LTE:
        case FilterOperation::GTE:
            ++metrics.predicate_type_metrics.num_range;
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
            ++metrics.predicate_type_metrics.num_string;
        } else {
            ++metrics.predicate_type_metrics.num_string_with_wildcard;
        }
    }
    if (operand->as_int(int_value, op) || operand->as_timestamp()) {
        ++metrics.predicate_type_metrics.num_integer;
    }
    if (operand->as_float(float_value, op)) {
        ++metrics.predicate_type_metrics.num_floating_point;
    }
    if (operand->as_null(op)) {
        ++metrics.predicate_type_metrics.num_null;
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
    // Constructors
    Impl()
            : m_span{opentelemetry::trace::Provider::GetTracerProvider()
                             ->GetTracer(to_nostd_string_view(cTracerName))
                             ->StartSpan(to_nostd_string_view(cSearchArchiveSpanName))},
              m_scope{std::make_unique<opentelemetry::trace::Scope>(m_span)} {
        m_span->SetAttribute(to_nostd_string_view(cAttrSuccess), true);
    }

    Impl(Impl const&) = delete;
    Impl(Impl&&) = delete;

    // Operators
    auto operator=(Impl const&) -> Impl& = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    // Destructor
    ~Impl() { m_span->End(); }

    // Methods
    auto set_archive_context(std::string_view archive_id) -> void {
        m_span->SetAttribute(
                to_nostd_string_view(cAttrArchiveIdHash),
                to_hash_attribute(archive_id)
        );
    }

    auto set_error(std::string_view message) -> void {
        m_span->SetAttribute(to_nostd_string_view(cAttrSuccess), false);
        m_span->SetAttribute(to_nostd_string_view(cAttrError), to_nostd_string_view(message));
        m_span->SetStatus(StatusCode::kError, to_nostd_string_view(message));
    }

    auto set_query_context(std::string_view query) -> void {
        m_span->SetAttribute(to_nostd_string_view(cAttrQueryHash), to_hash_attribute(query));
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
                to_int64_attribute(metrics.column_shape_metrics.num_pure_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrColumnTypesSomeWildcard),
                to_int64_attribute(metrics.column_shape_metrics.num_some_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrColumnTypesNoWildcard),
                to_int64_attribute(metrics.column_shape_metrics.num_no_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesString),
                to_int64_attribute(metrics.predicate_type_metrics.num_string)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesStringWithWildcard),
                to_int64_attribute(metrics.predicate_type_metrics.num_string_with_wildcard)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesInt),
                to_int64_attribute(metrics.predicate_type_metrics.num_integer)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesFloat),
                to_int64_attribute(metrics.predicate_type_metrics.num_floating_point)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesNull),
                to_int64_attribute(metrics.predicate_type_metrics.num_null)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesExactMatch),
                to_int64_attribute(metrics.predicate_type_metrics.num_exact_match)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesRange),
                to_int64_attribute(metrics.predicate_type_metrics.num_range)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrPredicateTypesExists),
                to_int64_attribute(metrics.predicate_type_metrics.num_exists)
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
                to_int64_attribute(metrics.num_total_archive_records)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrCandidateRecordsAfterSchemaMatching),
                to_int64_attribute(metrics.num_candidate_records_after_schema_matching)
        );
        m_span->SetAttribute(
                to_nostd_string_view(cAttrRecordsMatchingQuery),
                to_int64_attribute(metrics.num_records_matching_query)
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

private:
    // Data members
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
    std::unique_ptr<opentelemetry::trace::Scope> m_scope;
};

SearchTelemetrySpan::SearchTelemetrySpan() : m_impl{std::make_unique<Impl>()} {}

SearchTelemetrySpan::~SearchTelemetrySpan() = default;

auto SearchTelemetrySpan::set_archive_context(std::string_view archive_id) -> void {
    m_impl->set_archive_context(archive_id);
}

auto SearchTelemetrySpan::set_error(std::string_view message) -> void {
    m_impl->set_error(message);
}

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

auto QueryShapeMetrics::create(
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

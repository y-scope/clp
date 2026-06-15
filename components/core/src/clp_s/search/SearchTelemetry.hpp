#ifndef CLP_S_SEARCH_SEARCHTELEMETRY_HPP
#define CLP_S_SEARCH_SEARCHTELEMETRY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <clp_s/Defs.hpp>
#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search {
constexpr std::string_view cTerminationStageRangeIndexMatching{"range_index_matching"};
constexpr std::string_view cTerminationStageTimeRangeMatching{"time_range_matching"};
constexpr std::string_view cTerminationStageTimeRangeMatchingAfterColumnResolution{
        "time_range_matching_after_column_resolution"
};
constexpr std::string_view cTerminationStageSchemaMatching{"schema_matching"};
constexpr std::string_view cTerminationStageErtScan{"ert_scan"};
constexpr std::string_view cTerminationStageDictionarySearch{"dictionary_search"};

/**
 * Counts of how the columns referenced by a query's predicates use wildcards.
 */
struct ColumnShapeMetrics {
    uint64_t num_pure_wildcard{};
    uint64_t num_some_wildcard{};
    uint64_t num_no_wildcard{};
};

/**
 * Counts of the predicate operations and operand types referenced by a query.
 */
struct PredicateTypeMetrics {
    uint64_t num_string{};
    uint64_t num_string_with_wildcard{};
    uint64_t num_integer{};
    uint64_t num_floating_point{};
    uint64_t num_null{};
    uint64_t num_exact_match{};
    uint64_t num_range{};
    uint64_t num_exists{};
};

/**
 * Query-shape metrics derivable from the parsed query before any archive is searched.
 */
struct QueryShapeMetrics {
    // Factory methods
    /**
     * Walks the parsed query and accumulates its shape metrics.
     *
     * @param expr The parsed query AST.
     * @param search_begin_ts The lower bound of the search time range, if any.
     * @param search_end_ts The upper bound of the search time range, if any.
     * @return The query-shape metrics.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<ast::Expression> const& expr,
            std::optional<epochtime_t> search_begin_ts,
            std::optional<epochtime_t> search_end_ts
    ) -> QueryShapeMetrics;

    // Data members
    ColumnShapeMetrics column_shape_metrics;
    PredicateTypeMetrics predicate_type_metrics;
    uint64_t num_predicates{};
    bool contains_or_clause{};
    std::optional<uint64_t> time_range_millis;
};

/**
 * Record counts produced while searching a single archive.
 */
struct SearchResultMetrics {
    uint64_t num_total_archive_records{};
    uint64_t num_candidate_records_after_schema_matching{};
    uint64_t num_records_matching_query{};
    uint64_t num_matched_schemas{};
    uint64_t num_schemas_with_matches{};
};

/**
 * An OpenTelemetry span recording the telemetry for one archive search. The span starts on
 * construction and ends on destruction; each group of metrics is recorded as it becomes available
 * via the corresponding setter.
 */
class SearchTelemetrySpan {
public:
    // Constructors
    SearchTelemetrySpan();
    SearchTelemetrySpan(SearchTelemetrySpan const&) = delete;
    SearchTelemetrySpan(SearchTelemetrySpan&&) = delete;

    // Operators
    auto operator=(SearchTelemetrySpan const&) -> SearchTelemetrySpan& = delete;
    auto operator=(SearchTelemetrySpan&&) -> SearchTelemetrySpan& = delete;

    // Destructor
    ~SearchTelemetrySpan();

    // Methods
    /**
     * Records the archive-identity attributes: a non-reversible hash of the archive ID.
     *
     * @param archive_id The ID of the archive being searched.
     */
    auto set_archive_context(std::string_view archive_id) -> void;

    /**
     * Marks the span as failed and records the error.
     *
     * @param message
     */
    auto set_error(std::string_view message) -> void;

    /**
     * Records the query-identity attributes: a non-reversible hash of the query and, when present
     * in the environment, the scheduler's `query_id`/`task_id`.
     *
     * @param query The raw query string.
     */
    auto set_query_context(std::string_view query) -> void;

    /**
     * Records the query-shape metrics. These are collected from the preprocessed query (the form
     * that gets evaluated, in which e.g. EXISTS predicates appear).
     *
     * @param metrics
     */
    auto set_query_shape_metrics(QueryShapeMetrics const& metrics) -> void;

    /**
     * Records the record-count metrics gathered while searching the archive.
     *
     * @param metrics
     */
    auto set_search_result_metrics(SearchResultMetrics const& metrics) -> void;

    /**
     * Records the stage at which the search stopped processing the archive.
     *
     * @param termination_stage One of the `cTerminationStage*` constants.
     */
    auto set_termination_stage(std::string_view termination_stage) -> void;

private:
    // Types
    class Impl;

    // Data members
    std::unique_ptr<Impl> m_impl;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SEARCHTELEMETRY_HPP

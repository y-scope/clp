#ifndef CLP_S_SEARCH_SEARCHTELEMETRY_HPP
#define CLP_S_SEARCH_SEARCHTELEMETRY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <clp_s/Defs.hpp>
#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search {
constexpr std::string_view cTerminationStageRecordScan{"record_scan"};
constexpr std::string_view cTerminationStageRangeIndexMatching{"range_index_matching"};
constexpr std::string_view cTerminationStageTimeRangeMatching{"time_range_matching"};
constexpr std::string_view cTerminationStageTimeRangeMatchingAfterColumnResolution{
        "time_range_matching_after_column_resolution"
};
constexpr std::string_view cTerminationStageSchemaMatching{"schema_matching"};
constexpr std::string_view cTerminationStageErtScan{"ert_scan"};
constexpr std::string_view cTerminationStageDictionarySearch{"dictionary_search"};

/**
 * Telemetry collected while searching a single archive, recorded onto an OpenTelemetry span by
 * `SearchTelemetrySpan::set_telemetry`.
 */
struct SearchTelemetry {
    struct ColumnShapeMetrics {
        uint64_t pure_wildcard{};
        uint64_t some_wildcard{};
        uint64_t no_wildcard{};
    };

    struct PredicateTypeMetrics {
        uint64_t string{};
        uint64_t string_with_wildcard{};
        uint64_t integer{};
        uint64_t floating_point{};
        uint64_t null{};
        uint64_t exact_match{};
        uint64_t range{};
        uint64_t exists{};
    };

    std::string archive_id;
    std::string query_id;
    std::string task_id;
    uint64_t query_hash{};
    std::optional<std::string> query;

    ColumnShapeMetrics column_shape_metrics;
    PredicateTypeMetrics predicate_type_metrics;
    uint64_t num_predicates{};
    bool contains_or_clause{};
    std::optional<uint64_t> time_range_millis;

    uint64_t total_archive_records{};
    uint64_t candidate_records_after_schema_matching{};
    uint64_t records_matching_query{};

    uint64_t num_matched_schemas{};
    uint64_t num_schemas_with_matches{};

    std::string_view termination_stage{cTerminationStageRecordScan};
};

/**
 * Records the telemetry for one archive search as an OpenTelemetry span: the span starts on
 * construction and ends on destruction, and is populated via `set_telemetry` or `set_error`.
 */
class SearchTelemetrySpan {
public:
    // Constructors
    SearchTelemetrySpan();

    // Disable copy/move constructors and assignment operators
    SearchTelemetrySpan(SearchTelemetrySpan const&) = delete;
    SearchTelemetrySpan(SearchTelemetrySpan&&) = delete;
    auto operator=(SearchTelemetrySpan const&) -> SearchTelemetrySpan& = delete;
    auto operator=(SearchTelemetrySpan&&) -> SearchTelemetrySpan& = delete;

    // Destructor
    ~SearchTelemetrySpan();

    // Methods
    auto set_error(std::string_view message) -> void;
    auto set_telemetry(SearchTelemetry const& telemetry) -> void;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

[[nodiscard]] auto collect_query_shape_metrics(
        std::shared_ptr<ast::Expression> const& expr,
        std::optional<epochtime_t> search_begin_ts,
        std::optional<epochtime_t> search_end_ts
) -> SearchTelemetry;

/**
 * Populates the query identity/context fields of `telemetry`: the archive id, a non-reversible hash
 * of the query, and (from the environment) the scheduler's `query_id`/`task_id`. The raw query text
 * is included only when `CLP_TELEMETRY_INCLUDE_QUERY` is set to a truthy value.
 * @param telemetry
 * @param query The raw query string.
 * @param archive_id The id of the archive being searched.
 */
auto populate_query_context(
        SearchTelemetry& telemetry,
        std::string_view query,
        std::string_view archive_id
) -> void;
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SEARCHTELEMETRY_HPP

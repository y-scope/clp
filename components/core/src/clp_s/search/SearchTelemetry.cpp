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
#include <utility>

#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/nostd/string_view.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_metadata.h>
#include <opentelemetry/trace/tracer.h>

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
constexpr char cTracerName[]{"clp_s.search"};
constexpr char cSearchArchiveSpanName[]{"clp_s.search.archive"};

// Values (case-insensitive) treated as "true" for CLP's boolean environment variables.
constexpr std::array<std::string_view, 4> cTruthyEnvValues{"1", "true", "yes", "y"};

/**
 * @param name
 * @return Whether the given environment variable is set to a truthy value (one of
 * `cTruthyEnvValues`).
 */
[[nodiscard]] auto is_env_truthy(char const* name) -> bool;

/**
 * Computes the 64-bit FNV-1a hash of `data`. Used as a stable, non-reversible fingerprint to group
 * identical queries without exposing their content.
 * @param data
 * @return The hash.
 */
[[nodiscard]] auto fnv1a_64(std::string_view data) -> uint64_t;

/**
 * @param value
 * @return `value` as a zero-padded 16-character lowercase hex string.
 */
[[nodiscard]] auto to_hex64(uint64_t value) -> std::string;

/**
 * Sets a string-valued attribute on `span`.
 * @param span
 * @param key
 * @param value
 */
auto set_string_attribute(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const& span,
        opentelemetry::nostd::string_view key,
        std::string_view value
) -> void;

/**
 * @param value
 * @return `value` clamped to the maximum `int64_t`, since OpenTelemetry span attributes are signed.
 */
[[nodiscard]] auto to_int64_attribute(uint64_t value) -> int64_t;

/**
 * @param column
 * @return Whether any descriptor in the column is a wildcard.
 */
[[nodiscard]] auto descriptor_has_wildcard(ColumnDescriptor const& column) -> bool;

/**
 * Increments the column-shape counter in `telemetry` corresponding to `column`'s wildcard usage.
 * @param telemetry
 * @param column
 */
auto add_column_shape(SearchTelemetry& telemetry, ColumnDescriptor const& column) -> void;

/**
 * Increments the predicate-type counters in `telemetry` for `filter`'s operation and operand type.
 * @param telemetry
 * @param filter
 */
auto add_predicate_type(SearchTelemetry& telemetry, FilterExpr const& filter) -> void;

/**
 * Recursively walks `expr`, accumulating query-shape metrics (column shapes, predicate types,
 * predicate count, and whether an OR clause is present) into `telemetry`.
 * @param telemetry
 * @param expr
 */
auto collect_query_shape_metrics(
        SearchTelemetry& telemetry,
        std::shared_ptr<Expression> const& expr
) -> void;

/**
 * Sets a `uint64_t`-valued attribute on `span`, clamping the value to the signed range expected by
 * OpenTelemetry span attributes.
 * @param span
 * @param key
 * @param value
 */
auto set_uint64_attribute(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const& span,
        opentelemetry::nostd::string_view key,
        uint64_t value
) -> void;

auto is_env_truthy(char const* name) -> bool {
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

auto fnv1a_64(std::string_view data) -> uint64_t {
    constexpr uint64_t cOffsetBasis{14695981039346656037ULL};
    constexpr uint64_t cPrime{1099511628211ULL};
    uint64_t hash{cOffsetBasis};
    for (auto const c : data) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= cPrime;
    }
    return hash;
}

auto to_hex64(uint64_t value) -> std::string {
    constexpr char cHexDigits[]{"0123456789abcdef"};
    constexpr int cNumHexDigits{16};
    std::string result(cNumHexDigits, '0');
    for (int i{cNumHexDigits - 1}; i >= 0; --i) {
        result[static_cast<size_t>(i)] = cHexDigits[value & 0xFU];
        value >>= 4U;
    }
    return result;
}

auto set_string_attribute(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const& span,
        opentelemetry::nostd::string_view key,
        std::string_view value
) -> void {
    span->SetAttribute(key, opentelemetry::nostd::string_view{value.data(), value.size()});
}

auto to_int64_attribute(uint64_t value) -> int64_t {
    constexpr auto cMaxInt64{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
    return value > cMaxInt64 ? std::numeric_limits<int64_t>::max() : static_cast<int64_t>(value);
}

auto descriptor_has_wildcard(ColumnDescriptor const& column) -> bool {
    return std::ranges::any_of(
            column.descriptor_begin(),
            column.descriptor_end(),
            [](auto const& descriptor) { return descriptor.wildcard(); }
    );
}

auto add_column_shape(SearchTelemetry& telemetry, ColumnDescriptor const& column) -> void {
    if (column.is_pure_wildcard()) {
        ++telemetry.column_shape_metrics.pure_wildcard;
    } else if (descriptor_has_wildcard(column)) {
        ++telemetry.column_shape_metrics.some_wildcard;
    } else {
        ++telemetry.column_shape_metrics.no_wildcard;
    }
}

auto add_predicate_type(SearchTelemetry& telemetry, FilterExpr const& filter) -> void {
    auto const op{filter.get_operation()};
    switch (op) {
        case FilterOperation::EXISTS:
        case FilterOperation::NEXISTS:
            ++telemetry.predicate_type_metrics.exists;
            return;
        case FilterOperation::EQ:
        case FilterOperation::NEQ:
            ++telemetry.predicate_type_metrics.exact_match;
            break;
        case FilterOperation::LT:
        case FilterOperation::GT:
        case FilterOperation::LTE:
        case FilterOperation::GTE:
            ++telemetry.predicate_type_metrics.range;
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
            ++telemetry.predicate_type_metrics.string;
        } else {
            ++telemetry.predicate_type_metrics.string_with_wildcard;
        }
    }
    if (operand->as_int(int_value, op) || operand->as_timestamp()) {
        ++telemetry.predicate_type_metrics.integer;
    }
    if (operand->as_float(float_value, op)) {
        ++telemetry.predicate_type_metrics.floating_point;
    }
    if (operand->as_null(op)) {
        ++telemetry.predicate_type_metrics.null;
    }
    static_cast<void>(operand->as_bool(bool_value, op));
}

auto collect_query_shape_metrics(
        SearchTelemetry& telemetry,
        std::shared_ptr<Expression> const& expr
) -> void {
    if (nullptr == expr) {
        return;
    }
    if (nullptr != std::dynamic_pointer_cast<OrExpr>(expr)) {
        telemetry.contains_or_clause = true;
    }
    if (auto const filter{std::dynamic_pointer_cast<FilterExpr>(expr)}; nullptr != filter) {
        ++telemetry.num_predicates;
        add_column_shape(telemetry, *filter->get_column());
        add_predicate_type(telemetry, *filter);
        return;
    }
    for (auto it{expr->op_begin()}; it != expr->op_end(); ++it) {
        if (auto const child{std::dynamic_pointer_cast<Expression>(*it)}; nullptr != child) {
            collect_query_shape_metrics(telemetry, child);
        }
    }
}

auto set_uint64_attribute(
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const& span,
        opentelemetry::nostd::string_view key,
        uint64_t value
) -> void {
    span->SetAttribute(key, to_int64_attribute(value));
}
}  // namespace

class SearchTelemetrySpan::Impl {
public:
    Impl()
            : m_span{opentelemetry::trace::Provider::GetTracerProvider()
                             ->GetTracer(cTracerName)
                             ->StartSpan(cSearchArchiveSpanName)},
              m_scope{std::make_unique<opentelemetry::trace::Scope>(m_span)} {
        m_span->SetAttribute("clp.search.success", true);
    }

    ~Impl() { m_span->End(); }

    Impl(Impl const&) = delete;
    auto operator=(Impl const&) -> Impl& = delete;

    Impl(Impl&&) = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    auto set_error(std::string_view message) -> void {
        m_span->SetAttribute("clp.search.success", false);
        m_span->SetAttribute("clp.search.error", opentelemetry::nostd::string_view{
                                                         message.data(),
                                                         message.size()
                                                 });
        m_span->SetStatus(StatusCode::kError, opentelemetry::nostd::string_view{
                                                      message.data(),
                                                      message.size()
                                              });
    }

    auto set_telemetry(SearchTelemetry const& telemetry) -> void {
        if (false == telemetry.archive_id.empty()) {
            set_string_attribute(m_span, "clp.search.archive_id", telemetry.archive_id);
        }
        if (false == telemetry.query_id.empty()) {
            set_string_attribute(m_span, "clp.search.query_id", telemetry.query_id);
        }
        if (false == telemetry.task_id.empty()) {
            set_string_attribute(m_span, "clp.search.task_id", telemetry.task_id);
        }
        if (false == telemetry.query_hash.empty()) {
            set_string_attribute(m_span, "clp.search.query_hash", telemetry.query_hash);
        }
        if (telemetry.query.has_value()) {
            set_string_attribute(m_span, "clp.search.query", *telemetry.query);
        }

        // Integer counters (query-shape metrics and record counts).
        std::array<std::pair<opentelemetry::nostd::string_view, uint64_t>, 17> const
                uint64_attributes{
                        {{"clp.query_shape.column_types.pure_wildcard",
                          telemetry.column_shape_metrics.pure_wildcard},
                         {"clp.query_shape.column_types.some_wildcard",
                          telemetry.column_shape_metrics.some_wildcard},
                         {"clp.query_shape.column_types.no_wildcard",
                          telemetry.column_shape_metrics.no_wildcard},
                         {"clp.query_shape.predicate_types.string",
                          telemetry.predicate_type_metrics.string},
                         {"clp.query_shape.predicate_types.string_with_wildcard",
                          telemetry.predicate_type_metrics.string_with_wildcard},
                         {"clp.query_shape.predicate_types.int",
                          telemetry.predicate_type_metrics.integer},
                         {"clp.query_shape.predicate_types.float",
                          telemetry.predicate_type_metrics.floating_point},
                         {"clp.query_shape.predicate_types.null",
                          telemetry.predicate_type_metrics.null},
                         {"clp.query_shape.predicate_types.exact_match",
                          telemetry.predicate_type_metrics.exact_match},
                         {"clp.query_shape.predicate_types.range",
                          telemetry.predicate_type_metrics.range},
                         {"clp.query_shape.predicate_types.exists",
                          telemetry.predicate_type_metrics.exists},
                         {"clp.query_shape.num_predicates", telemetry.num_predicates},
                         {"clp.search.total_archive_records", telemetry.total_archive_records},
                         {"clp.search.candidate_records_after_schema_matching",
                          telemetry.candidate_records_after_schema_matching},
                         {"clp.search.records_matching_query", telemetry.records_matching_query},
                         {"clp.search.num_matched_schemas", telemetry.num_matched_schemas},
                         {"clp.search.num_schemas_with_matches",
                          telemetry.num_schemas_with_matches}}
                };
        for (auto const& [key, value] : uint64_attributes) {
            set_uint64_attribute(m_span, key, value);
        }

        m_span->SetAttribute("clp.query_shape.contains_or_clause", telemetry.contains_or_clause);
        if (telemetry.time_range_millis.has_value()) {
            set_uint64_attribute(
                    m_span,
                    "clp.query_shape.time_range_millis",
                    *telemetry.time_range_millis
            );
        }
        set_string_attribute(m_span, "clp.search.termination_stage", telemetry.termination_stage);
    }

private:
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> m_span;
    std::unique_ptr<opentelemetry::trace::Scope> m_scope;
};

SearchTelemetrySpan::SearchTelemetrySpan() : m_impl{std::make_unique<Impl>()} {}

SearchTelemetrySpan::~SearchTelemetrySpan() = default;

auto SearchTelemetrySpan::set_error(std::string_view message) -> void {
    m_impl->set_error(message);
}

auto SearchTelemetrySpan::set_telemetry(SearchTelemetry const& telemetry) -> void {
    m_impl->set_telemetry(telemetry);
}

auto collect_query_shape_metrics(
        std::shared_ptr<ast::Expression> const& expr,
        std::optional<epochtime_t> search_begin_ts,
        std::optional<epochtime_t> search_end_ts
) -> SearchTelemetry {
    SearchTelemetry telemetry;
    collect_query_shape_metrics(telemetry, expr);
    if (search_begin_ts.has_value() && search_end_ts.has_value()) {
        auto const time_range_millis{*search_end_ts - *search_begin_ts};
        if (0 <= time_range_millis) {
            telemetry.time_range_millis = static_cast<uint64_t>(time_range_millis);
        }
    }
    return telemetry;
}

auto populate_query_context(
        SearchTelemetry& telemetry,
        std::string_view query,
        std::string_view archive_id
) -> void {
    telemetry.archive_id = std::string{archive_id};
    telemetry.query_hash = to_hex64(fnv1a_64(query));
    if (char const* const query_id{std::getenv("CLP_QUERY_ID")}; nullptr != query_id) {
        telemetry.query_id = query_id;
    }
    if (char const* const task_id{std::getenv("CLP_TASK_ID")}; nullptr != task_id) {
        telemetry.task_id = task_id;
    }
    if (is_env_truthy("CLP_TELEMETRY_INCLUDE_QUERY")) {
        telemetry.query = std::string{query};
    }
}
}  // namespace clp_s::search

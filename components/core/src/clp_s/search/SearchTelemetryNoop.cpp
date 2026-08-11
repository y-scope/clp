#include "SearchTelemetry.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace clp_s::search {
class SearchTelemetrySpan::Impl {};

SearchTelemetrySpan::SearchTelemetrySpan() : m_impl{std::make_unique<Impl>()} {}

SearchTelemetrySpan::~SearchTelemetrySpan() = default;

auto SearchTelemetrySpan::set_archive_context(std::string_view) -> void {}

auto SearchTelemetrySpan::set_error(std::string_view) -> void {}

auto SearchTelemetrySpan::set_query_context(std::string_view) -> void {}

auto SearchTelemetrySpan::set_query_shape_metrics(QueryShapeMetrics const&) -> void {}

auto SearchTelemetrySpan::set_search_result_metrics(SearchResultMetrics const&) -> void {}

auto SearchTelemetrySpan::set_termination_stage(std::string_view) -> void {}

auto QueryShapeMetrics::create(
        std::shared_ptr<ast::Expression> const&,
        std::optional<epochtime_t>,
        std::optional<epochtime_t>
) -> QueryShapeMetrics {
    return {};
}
}  // namespace clp_s::search

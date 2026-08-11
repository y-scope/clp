#include "TelemetryContext.hpp"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer_provider.h>
#include <spdlog/spdlog.h>

namespace otlp = opentelemetry::exporter::otlp;
namespace ostream_exporter = opentelemetry::exporter::trace;
namespace resource = opentelemetry::sdk::resource;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;

namespace clp_s::search {
namespace {
constexpr std::string_view cServiceNameKey{"service.name"};
constexpr std::string_view cDefaultServiceName{"clp-search"};
constexpr std::string_view cTracesPath{"/v1/traces"};

/**
 * Sentinel value for `CLP_TELEMETRY_ENDPOINT` that selects the OStream span exporter (writes spans
 * to `std::cerr`) instead of the OTLP HTTP exporter.
 */
constexpr std::string_view cOStreamEndpoint{"ostream"};

/**
 * Bound on how long the destructor blocks while draining buffered spans on exit.
 */
constexpr std::chrono::seconds cShutdownTimeout{1};

/**
 * @param name
 * @return Whether the given environment variable is set to a non-empty value.
 */
[[nodiscard]] auto is_env_set(char const* name) -> bool;

/**
 * Resolves the OTLP traces endpoint from CLP-specific configuration.
 *
 * The standard `OTEL_EXPORTER_OTLP_TRACES_ENDPOINT`/`OTEL_EXPORTER_OTLP_ENDPOINT` variables are
 * honoured natively by `OtlpHttpExporterOptions`, so this only supplies an override derived from
 * CLP's `CLP_TELEMETRY_ENDPOINT` when none of the standard variables are set.
 *
 * If `CLP_TELEMETRY_ENDPOINT` is set to the sentinel value `cOStreamEndpoint` ("ostream"), the
 * OStream span exporter is used instead of the OTLP HTTP exporter, writing spans to `std::cerr`.
 * This is useful for local debugging and evaluation without an OTLP collector.
 *
 * @return The fully-qualified traces URL, the sentinel `cOStreamEndpoint`, or std::nullopt if
 * opentelemetry defaults should be used.
 */
[[nodiscard]] auto resolve_clp_endpoint() -> std::optional<std::string>;

/**
 * @return A resource describing this process; a default `service.name` of `clp-search`
 * (used only when `OTEL_SERVICE_NAME` is unset) merged with the attributes detected from the
 * environment.
 */
[[nodiscard]] auto make_resource() -> resource::Resource;

auto is_env_set(char const* name) -> bool {
    char const* const raw{std::getenv(name)};
    return nullptr != raw && '\0' != raw[0];
}

auto resolve_clp_endpoint() -> std::optional<std::string> {
    if (is_env_set("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT")
        || is_env_set("OTEL_EXPORTER_OTLP_ENDPOINT"))
    {
        return std::nullopt;
    }

    if (false == is_env_set("CLP_TELEMETRY_ENDPOINT")) {
        return std::nullopt;
    }

    std::string endpoint{std::getenv("CLP_TELEMETRY_ENDPOINT")};
    while (false == endpoint.empty() && '/' == endpoint.back()) {
        endpoint.pop_back();
    }
    if (endpoint.empty()) {
        return std::nullopt;
    }
    if (cOStreamEndpoint == endpoint) {
        return endpoint;
    }
    endpoint.append(cTracesPath);
    return endpoint;
}

auto make_resource() -> resource::Resource {
    resource::ResourceAttributes attributes{};
    if (false == is_env_set("OTEL_SERVICE_NAME")) {
        attributes.SetAttribute(std::string{cServiceNameKey}, std::string{cDefaultServiceName});
    }
    return resource::Resource::Create(attributes);
}
}  // namespace

class TelemetryContext::Impl {
public:
    // Constructors
    Impl() {
        try {
            std::unique_ptr<trace_sdk::SpanExporter> exporter;
            auto const endpoint{resolve_clp_endpoint()};
            if (endpoint.has_value() && cOStreamEndpoint == *endpoint) {
                exporter = ostream_exporter::OStreamSpanExporterFactory::Create(std::cerr);
            } else {
                otlp::OtlpHttpExporterOptions exporter_options{};
                if (endpoint.has_value()) {
                    exporter_options.url = *endpoint;
                }
                exporter_options.content_type = otlp::HttpRequestContentType::kBinary;
                exporter = otlp::OtlpHttpExporterFactory::Create(exporter_options);
            }

            trace_sdk::BatchSpanProcessorOptions const processor_options{};
            auto processor{trace_sdk::BatchSpanProcessorFactory::Create(
                    std::move(exporter),
                    processor_options
            )};
            std::shared_ptr<trace_api::TracerProvider> const provider{
                    trace_sdk::TracerProviderFactory::Create(std::move(processor), make_resource())
            };

            m_provider = provider;
            trace_api::Provider::SetTracerProvider(provider);
        } catch (std::exception const& e) {
            SPDLOG_WARN("Failed to initialize the telemetry exporter - {}", e.what());
        }
    }

    // Delete copy constructor and assignment operator
    Impl(Impl const&) = delete;
    auto operator=(Impl const&) -> Impl& = delete;

    // Delete move constructor and assignment operator
    Impl(Impl&&) = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    // Destructor
    ~Impl() {
        if (nullptr == m_provider) {
            return;
        }
        auto* const sdk_provider{static_cast<trace_sdk::TracerProvider*>(m_provider.get())};
        sdk_provider->Shutdown(cShutdownTimeout);
        std::shared_ptr<trace_api::TracerProvider> const none{};
        trace_api::Provider::SetTracerProvider(none);
    }

private:
    // Data members
    std::shared_ptr<trace_api::TracerProvider> m_provider;
};

TelemetryContext::TelemetryContext() : m_impl{std::make_unique<Impl>()} {}

TelemetryContext::~TelemetryContext() = default;
}  // namespace clp_s::search

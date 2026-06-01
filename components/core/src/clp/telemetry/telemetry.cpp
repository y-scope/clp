#include "telemetry.hpp"

#include <cstdlib>
#include <iostream>

#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter_options.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>

namespace clp::telemetry {

namespace {
bool is_disabled() {
    auto check_var = [](const char* name) {
        if (const char* val = std::getenv(name)) {
            std::string s(val);
            if (s == "1" || s == "true" || s == "yes" || s == "y") {
                return true;
            }
        }
        return false;
    };
    return check_var("CLP_DISABLE_TELEMETRY") || check_var("DO_NOT_TRACK");
}
}

void init() {
    if (is_disabled()) {
        return;
    }

    opentelemetry::exporter::otlp::OtlpHttpMetricExporterOptions opts;
    if (const char* endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT")) {
        opts.url = std::string(endpoint) + "/v1/metrics";
    } else {
        opts.url = "http://localhost:4318/v1/metrics";
    }

    auto exporter = opentelemetry::exporter::otlp::OtlpHttpMetricExporterFactory::Create(opts);

    opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions reader_options;
    auto reader = opentelemetry::sdk::metrics::PeriodicExportingMetricReaderFactory::Create(
            std::move(exporter), reader_options);

    auto provider = opentelemetry::sdk::metrics::MeterProviderFactory::Create();
    auto* p = static_cast<opentelemetry::sdk::metrics::MeterProvider*>(provider.get());
    p->AddMetricReader(std::move(reader));

    opentelemetry::metrics::Provider::SetMeterProvider(std::move(provider));
}

void shutdown() {
    auto provider = opentelemetry::metrics::Provider::GetMeterProvider();
    if (provider) {
        opentelemetry::metrics::Provider::SetMeterProvider(nullptr);
    }
}

void record_query_metrics(uint64_t bytes_scanned, uint64_t bytes_output) {
    auto provider = opentelemetry::metrics::Provider::GetMeterProvider();
    if (!provider) {
        return;
    }
    auto meter = provider->GetMeter("query-worker");

    auto scanned_counter = meter->CreateUInt64Counter("clp.query.bytes_scanned_total");
    scanned_counter->Add(bytes_scanned);

    auto output_counter = meter->CreateUInt64Counter("clp.query.bytes_output_total");
    output_counter->Add(bytes_output);
}

} // namespace clp::telemetry

#include "ClpArchiveDecoder.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>
#include <clp_s/SchemaReader.hpp>
#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/QueryRunner.hpp>
#include <clp_s/search/SchemaMatch.hpp>

#include "KqlQuery.hpp"
#include "SfaErrorCode.hpp"

namespace clp_s::ffi::sfa {
auto ClpArchiveDecoder::create(clp_s::ArchiveReader& reader)
        -> ystdlib::error_handling::Result<ClpArchiveDecoder> {
    try {
        return ClpArchiveDecoder{reader.read_all_tables()};
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}

auto ClpArchiveDecoder::create(
        clp_s::ArchiveReader& reader,
        std::shared_ptr<clp_s::search::SchemaMatch> schema_match,
        KqlQuery const& query
) -> ystdlib::error_handling::Result<ClpArchiveDecoder> {
    try {
        auto expression{query.copy_expression()};
        auto matched_expression{schema_match->run(expression)};

        if (std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(matched_expression)) {
            SPDLOG_ERROR("KQL query does not match any schemas in the archive.");
            return SfaErrorCode{SfaErrorCodeEnum::Failure};
        }

        auto query_runner = std::make_unique<clp_s::search::QueryRunner>(
                schema_match,
                matched_expression,
                std::shared_ptr<clp_s::ArchiveReader>{&reader, [](clp_s::ArchiveReader*) {}},
                query.ignore_case()
        );

        query_runner->global_init();
        auto all_tables{reader.read_all_tables()};
        std::vector<std::shared_ptr<clp_s::SchemaReader>> filtered_tables;
        filtered_tables.reserve(all_tables.size());

        for (auto const& table : all_tables) {
            if (clp_s::EvaluatedValue::False == query_runner->schema_init(table->get_schema_id())) {
                continue;
            }
            filtered_tables.push_back(table);
        }

        return ClpArchiveDecoder{std::move(filtered_tables), std::move(query_runner)};
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create filtered ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating filtered ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}

ClpArchiveDecoder::ClpArchiveDecoder(
        std::vector<std::shared_ptr<clp_s::SchemaReader>>&& tables,
        std::unique_ptr<clp_s::search::QueryRunner> query_runner
)
        : m_tables{std::move(tables)},
          m_query_runner{std::move(query_runner)} {}

ClpArchiveDecoder::ClpArchiveDecoder(ClpArchiveDecoder&& rhs) noexcept {
    move_from(rhs);
}

auto ClpArchiveDecoder::operator=(ClpArchiveDecoder&& rhs) noexcept -> ClpArchiveDecoder& {
    if (this == &rhs) {
        return *this;
    }

    close();
    move_from(rhs);
    return *this;
}

auto ClpArchiveDecoder::close() noexcept -> void {
    m_tables.clear();
    m_log_events.clear();
    m_query_runner.reset();
    m_decode_started = false;
    m_decode_completed = false;
}

auto ClpArchiveDecoder::move_from(ClpArchiveDecoder& rhs) noexcept -> void {
    m_tables = std::move(rhs.m_tables);
    m_log_events = std::move(rhs.m_log_events);
    m_query_runner = std::move(rhs.m_query_runner);
    m_decode_started = std::exchange(rhs.m_decode_started, false);
    m_decode_completed = std::exchange(rhs.m_decode_completed, false);
}

ClpArchiveDecoder::~ClpArchiveDecoder() {
    close();
}

auto ClpArchiveDecoder::decode_all() -> ystdlib::error_handling::Result<void> {
    if (m_decode_completed) {
        return ystdlib::error_handling::success();
    }

    m_decode_started = true;
    m_decode_completed = false;

    try {
        m_log_events.clear();

        for (auto const& table : m_tables) {
            std::string message;
            epochtime_t timestamp{0};
            int64_t log_event_idx{0};

            if (nullptr != m_query_runner) {
                m_query_runner->schema_init(table->get_schema_id());
                table->initialize_filter(*m_query_runner);
                while (table->get_next_message_with_metadata(
                        message,
                        timestamp,
                        log_event_idx,
                        *m_query_runner
                ))
                {
                    m_log_events.emplace_back(log_event_idx, timestamp, message);
                }
                continue;
            }

            while (table->get_next_message_with_metadata(message, timestamp, log_event_idx)) {
                m_log_events.emplace_back(log_event_idx, timestamp, message);
            }
        }

        m_decode_completed = true;
        return ystdlib::error_handling::success();
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to decode all log events in ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while decoding all log events in ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}
}  // namespace clp_s::ffi::sfa

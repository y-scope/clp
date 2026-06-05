#include "Output.hpp"

#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "../../clp/type_utils.hpp"
#include "../SchemaTree.hpp"
#include "../Utils.hpp"
#include "ast/AndExpr.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"
#include "EvaluateTimestampIndex.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::DescriptorList;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Literal;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::OpList;
using clp_s::search::ast::OrExpr;

#define eval(op, a, b) (((op) == FilterOperation::EQ) ? ((a) == (b)) : ((a) != (b)))

namespace clp_s::search {
bool Output::filter() {
    std::vector<int32_t> matched_schemas;
    bool has_array = false;
    bool has_array_search = false;

    if (auto const result{m_archive_reader->read_metadata()}; result.has_error()) {
        auto const error{result.error()};
        SPDLOG_ERROR(
                "Failed to read archive metadata: {} - {}",
                error.category().name(),
                error.message()
        );
        return false;
    }

    for (auto schema_id : m_archive_reader->get_schema_ids()) {
        if (nullptr != m_telemetry) {
            m_telemetry->total_archive_records
                    += m_archive_reader->get_schema_num_messages(schema_id);
        }
        if (m_match->schema_matched(schema_id)) {
            matched_schemas.push_back(schema_id);
            if (nullptr != m_telemetry) {
                m_telemetry->candidate_records_after_schema_matching
                        += m_archive_reader->get_schema_num_messages(schema_id);
            }
            if (m_match->has_array(schema_id)) {
                has_array = true;
            }
            if (m_match->has_array_search(schema_id)) {
                has_array_search = true;
            }
        }
    }
    if (nullptr != m_telemetry) {
        m_telemetry->num_matched_schemas = matched_schemas.size();
    }

    // Skip decompressing archive if it contains no
    // relevant schemas
    if (matched_schemas.empty()) {
        if (nullptr != m_telemetry) {
            m_telemetry->termination_stage = cTerminationStageSchemaMatching;
        }
        return true;
    }

    // Skip decompressing the rest of the archive if it won't match based on the timestamp range
    // index. This check happens a second time here because some ambiguous columns may now match the
    // timestamp column after column resolution.
    EvaluateTimestampIndex timestamp_index(m_archive_reader->get_timestamp_dictionary());
    if (EvaluatedValue::False == timestamp_index.run(m_expr)) {
        if (nullptr != m_telemetry) {
            m_telemetry->termination_stage
                    = cTerminationStageTimeRangeMatchingAfterColumnResolution;
        }
        m_archive_reader->close();
        return true;
    }

    m_archive_reader->read_variable_dictionary();
    m_archive_reader->read_log_type_dictionary();

    if (has_array) {
        if (has_array_search) {
            m_archive_reader->read_array_dictionary();
        } else {
            m_archive_reader->read_array_dictionary(true);
        }
    }

    m_query_runner.global_init();
    m_archive_reader->open_packed_streams();

    std::string message;
    auto const archive_id = m_archive_reader->get_archive_id();
    bool searched_schema{false};
    for (int32_t schema_id : matched_schemas) {
        if (EvaluatedValue::False == m_query_runner.schema_init(schema_id)) {
            continue;
        }
        searched_schema = true;

        auto& reader = m_archive_reader->read_schema_table(
                schema_id,
                m_output_handler->should_output_metadata(),
                m_should_marshal_records
        );
        reader.initialize_filter(m_query_runner);

        uint64_t schema_matched_records{};
        if (m_output_handler->should_output_metadata()) {
            epochtime_t timestamp{};
            int64_t log_event_idx{};
            while (reader.get_next_message_with_metadata(
                    message,
                    timestamp,
                    log_event_idx,
                    m_query_runner
            ))
            {
                ++schema_matched_records;
                m_output_handler->write(message, timestamp, archive_id, log_event_idx);
            }
        } else {
            while (reader.get_next_message(message, m_query_runner)) {
                ++schema_matched_records;
                m_output_handler->write(message);
            }
        }
        if (nullptr != m_telemetry) {
            m_telemetry->records_matching_query += schema_matched_records;
            if (0 != schema_matched_records) {
                ++m_telemetry->num_schemas_with_matches;
            }
        }
        auto ecode = m_output_handler->flush();
        if (ErrorCode::ErrorCodeSuccess != ecode) {
            SPDLOG_ERROR(
                    "Failed to flush output handler, error={}.",
                    clp::enum_to_underlying_type(ecode)
            );
            return false;
        }
    }
    if (nullptr != m_telemetry) {
        m_telemetry->termination_stage
                = searched_schema ? cTerminationStageErtScan : cTerminationStageDictionarySearch;
    }
    auto ecode = m_output_handler->finish();
    if (ErrorCode::ErrorCodeSuccess != ecode) {
        SPDLOG_ERROR(
                "Failed to flush output handler, error={}.",
                clp::enum_to_underlying_type(ecode)
        );
        return false;
    }
    return true;
}
}  // namespace clp_s::search

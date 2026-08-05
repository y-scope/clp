#include "ClpArchiveReader.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <istream>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/BufferReader.hpp>
#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/ffi/sfa/LogEvent.hpp>
#include <clp_s/ffi/sfa/SfaErrorCode.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>
#include <clp_s/search/ast/SetTimestampLiteralPrecision.hpp>
#include <clp_s/search/EvaluateRangeIndexFilters.hpp>
#include <clp_s/search/kql/kql.hpp>
#include <clp_s/search/Output.hpp>
#include <clp_s/search/OutputHandler.hpp>
#include <clp_s/search/SchemaMatch.hpp>

namespace {
class LogEventIndexOutputHandler : public clp_s::search::OutputHandler {
public:
    explicit LogEventIndexOutputHandler(
            std::vector<int64_t>& log_event_indices,
            std::optional<std::pair<int64_t, int64_t>> selected_range
    )
            : OutputHandler{true, false},
              m_log_event_indices{log_event_indices},
              m_selected_range{selected_range} {}

    auto write(std::string_view, clp_s::epochtime_t, std::string_view, int64_t log_event_idx)
            -> void override {
        if (m_selected_range.has_value()
            && (log_event_idx < m_selected_range->first
                || log_event_idx >= m_selected_range->second))
        {
            return;
        }
        m_log_event_indices.emplace_back(log_event_idx);
    }

    auto write(std::string_view) -> void override {}

private:
    std::vector<int64_t>& m_log_event_indices;
    std::optional<std::pair<int64_t, int64_t>> m_selected_range;
};

class ArchiveReaderCloseGuard {
public:
    explicit ArchiveReaderCloseGuard(std::shared_ptr<clp_s::ArchiveReader> reader)
            : m_reader{std::move(reader)} {}

    ~ArchiveReaderCloseGuard() {
        try {
            m_reader->close();
        } catch (std::exception const&) {
            // `Output::filter` may close the reader when timestamp-index evaluation determines
            // that the query cannot match.
        }
    }

private:
    std::shared_ptr<clp_s::ArchiveReader> m_reader;
};
}  // namespace

namespace clp_s::ffi::sfa {
template <typename ReturnType>
using Result = ystdlib::error_handling::Result<ReturnType>;

auto ClpArchiveReader::create(std::string_view archive_path) -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> reader;

    try {
        auto path{get_path_object_for_raw_path(archive_path)};
        reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});
        auto clp_archive_reader{
                ClpArchiveReader{std::move(reader), nullptr, std::string{archive_path}}
        };
        YSTDLIB_ERROR_HANDLING_TRYV(clp_archive_reader.precompute_archive_metadata());
        return clp_archive_reader;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR(
                "Failed to create ClpArchiveReader for archive {}: out of memory.",
                archive_path
        );
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

auto ClpArchiveReader::create(std::vector<char>&& archive_data) -> Result<ClpArchiveReader> {
    // `clp_s::ArchiveReader` requires an archive ID, but `clp_s::ffi::sfa::ClpArchiveReader` never
    // uses it. Provide a dummy value solely to satisfy the constructor.
    constexpr std::string_view cDefaultArchiveId{"default"};

    std::unique_ptr<clp_s::ArchiveReader> archive_reader;
    std::shared_ptr<std::vector<char>> archive_data_owner;

    try {
        archive_data_owner = std::make_shared<std::vector<char>>(std::move(archive_data));
        auto reader{std::make_shared<clp::BufferReader>(
                archive_data_owner->data(),
                archive_data_owner->size()
        )};

        archive_reader = std::make_unique<clp_s::ArchiveReader>();
        archive_reader->open(reader, cDefaultArchiveId);
        auto clp_archive_reader{ClpArchiveReader{
                std::move(archive_reader),
                std::move(archive_data_owner),
                std::string{}
        }};
        YSTDLIB_ERROR_HANDLING_TRYV(clp_archive_reader.precompute_archive_metadata());
        return clp_archive_reader;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create ClpArchiveReader: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

ClpArchiveReader::ClpArchiveReader(
        std::unique_ptr<clp_s::ArchiveReader> reader,
        std::shared_ptr<std::vector<char>> archive_data,
        std::string archive_path
)
        : m_archive_reader{std::move(reader)},
          m_archive_data{std::move(archive_data)},
          m_archive_path{std::move(archive_path)} {}

ClpArchiveReader::ClpArchiveReader(ClpArchiveReader&& rhs) noexcept {
    move_from(rhs);
}

auto ClpArchiveReader::operator=(ClpArchiveReader&& rhs) noexcept -> ClpArchiveReader& {
    if (this == &rhs) {
        return *this;
    }

    close();
    move_from(rhs);
    return *this;
}

ClpArchiveReader::~ClpArchiveReader() noexcept {
    close();
}

auto ClpArchiveReader::close() noexcept -> void {
    // FFI frontends may invoke destruction paths multiple times (e.g., explicit close followed by
    // GC finalization). Guard against this by checking for a null reader before attempting to
    // close.
    if (nullptr != m_archive_reader) {
        try {
            m_archive_reader->close();
        } catch (std::exception const& ex) {
            SPDLOG_ERROR("Exception while closing ClpArchiveReader: {}", ex.what());
        }
        m_archive_reader.reset();
    }
    m_archive_data.reset();
    m_archive_path.clear();
    m_event_count = 0;
    m_uncompressed_size = 0;
    m_file_names.clear();
    m_file_infos.clear();
    m_selected_file_info.reset();
    m_tables.clear();
    m_log_events.clear();
    m_is_decode_prepared = false;
    m_decode_state = DecodeState::NotStarted;
    m_decode_error.clear();
}

auto ClpArchiveReader::move_from(ClpArchiveReader& rhs) noexcept -> void {
    m_archive_reader = std::move(rhs.m_archive_reader);
    m_archive_data = std::move(rhs.m_archive_data);
    m_archive_path = std::move(rhs.m_archive_path);
    rhs.m_archive_path.clear();
    m_event_count = std::exchange(rhs.m_event_count, 0);
    m_uncompressed_size = std::exchange(rhs.m_uncompressed_size, 0);
    m_file_names = std::move(rhs.m_file_names);
    m_file_infos = std::move(rhs.m_file_infos);
    m_selected_file_info = std::move(rhs.m_selected_file_info);
    rhs.m_selected_file_info.reset();
    m_tables = std::move(rhs.m_tables);
    m_log_events = std::move(rhs.m_log_events);
    m_is_decode_prepared = std::exchange(rhs.m_is_decode_prepared, false);
    m_decode_state = std::exchange(rhs.m_decode_state, DecodeState::NotStarted);
    m_decode_error = std::exchange(rhs.m_decode_error, std::error_code{});
}

auto ClpArchiveReader::find_file_info(std::string_view file_name) const -> std::optional<FileInfo> {
    auto const it{std::find_if(
            m_file_infos.cbegin(),
            m_file_infos.cend(),
            [file_name](FileInfo const& file_info) {
                return file_info.get_file_name() == file_name;
            }
    )};
    if (m_file_infos.cend() == it) {
        return std::nullopt;
    }
    return *it;
}

auto ClpArchiveReader::select_file(std::string_view file_name) -> Result<void> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }
    if (DecodeState::NotStarted != m_decode_state || m_is_decode_prepared) {
        return SfaErrorCode{SfaErrorCodeEnum::FileSelectionAfterDecode};
    }

    auto const file_info{find_file_info(file_name)};
    if (false == file_info.has_value()) {
        return SfaErrorCode{SfaErrorCodeEnum::FileNotFound};
    }
    m_selected_file_info = file_info;
    return ystdlib::error_handling::success();
}

auto ClpArchiveReader::decode_all() -> Result<LogEventView> {
    return YSTDLIB_ERROR_HANDLING_TRYX(
            decode_range(0, static_cast<size_t>(get_active_event_count()))
    );
}

auto ClpArchiveReader::decode() -> Result<void> {
    if (DecodeState::Failed == m_decode_state) {
        return m_decode_error;
    }

    if (DecodeState::Decoded == m_decode_state) {
        return ystdlib::error_handling::success();
    }

    auto const decode_result{internal_decode_all()};
    if (decode_result.has_error()) {
        m_log_events.clear();
        m_decode_error = decode_result.error();
        m_decode_state = DecodeState::Failed;
        return m_decode_error;
    }
    m_decode_state = DecodeState::Decoded;
    return ystdlib::error_handling::success();
}

auto ClpArchiveReader::decode_range(size_t begin_idx, size_t end_idx) -> Result<LogEventView> {
    if (DecodeState::Failed == m_decode_state) {
        return m_decode_error;
    }

    if (begin_idx > end_idx || end_idx > get_active_event_count()) {
        return SfaErrorCode{SfaErrorCodeEnum::DecodeRangeOutOfBounds};
    }

    YSTDLIB_ERROR_HANDLING_TRYV(decode());

    return LogEventView{m_log_events}.subspan(begin_idx, end_idx - begin_idx);
}

auto ClpArchiveReader::search(std::string_view kql, bool ignore_case)
        -> Result<std::vector<size_t>> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }
    if (false == m_archive_reader->has_log_order()) {
        return SfaErrorCode{SfaErrorCodeEnum::LogEventIndexUnavailable};
    }

    YSTDLIB_ERROR_HANDLING_TRYV(decode());

    try {
        std::istringstream query_stream{std::string{kql}};
        auto expr{clp_s::search::kql::parse_kql_expression(query_stream)};
        if (nullptr == expr) {
            return SfaErrorCode{SfaErrorCodeEnum::InvalidQuery};
        }

        if (expr = clp_s::search::ast::preprocess_query(std::move(expr));
            std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr))
        {
            return std::vector<size_t>{};
        }

        auto search_reader{std::make_shared<clp_s::ArchiveReader>()};
        if (nullptr != m_archive_data) {
            // `m_archive_data` owns the memory referenced by the temporary buffer reader.
            auto buffer_reader{std::make_shared<clp::BufferReader>(
                    m_archive_data->data(),
                    m_archive_data->size()
            )};
            search_reader->open(std::move(buffer_reader), "default");
        } else {
            auto path{get_path_object_for_raw_path(m_archive_path)};
            search_reader->open(path, NetworkAuthOption{});
        }
        ArchiveReaderCloseGuard const close_guard{search_reader};

        clp_s::search::EvaluateRangeIndexFilters metadata_filter_pass{
                search_reader->get_range_index(),
                false == ignore_case
        };
        if (expr = metadata_filter_pass.run(expr);
            std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr))
        {
            return std::vector<size_t>{};
        }

        if (search_reader->has_deprecated_timestamp_format()) {
            clp_s::search::ast::SetTimestampLiteralPrecision date_precision_pass{
                    clp_s::search::ast::TimestampLiteral::Precision::Milliseconds
            };
            expr = date_precision_pass.run(expr);
        }

        auto match_pass{std::make_shared<clp_s::search::SchemaMatch>(
                search_reader->get_schema_tree(),
                search_reader->get_schema_map()
        )};
        if (expr = match_pass->run(expr);
            std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr))
        {
            return std::vector<size_t>{};
        }

        std::vector<int64_t> global_log_event_indices;
        std::optional<std::pair<int64_t, int64_t>> selected_range;
        if (m_selected_file_info.has_value()) {
            selected_range.emplace(
                    m_selected_file_info->get_start_index(),
                    m_selected_file_info->get_end_index()
            );
        }
        auto output_handler{std::make_unique<LogEventIndexOutputHandler>(
                global_log_event_indices,
                selected_range
        )};
        clp_s::search::Output
                output{match_pass, expr, search_reader, std::move(output_handler), ignore_case};
        if (false == output.filter()) {
            return SfaErrorCode{SfaErrorCodeEnum::SearchFailure};
        }

        std::sort(global_log_event_indices.begin(), global_log_event_indices.end());
        auto const unique_end{
                std::unique(global_log_event_indices.begin(), global_log_event_indices.end())
        };
        global_log_event_indices.erase(unique_end, global_log_event_indices.end());

        std::vector<size_t> matching_indices;
        matching_indices.reserve(global_log_event_indices.size());
        size_t decoded_idx{0};
        for (auto const global_idx : global_log_event_indices) {
            while (decoded_idx < m_log_events.size()
                   && m_log_events[decoded_idx].get_log_event_idx() < global_idx)
            {
                ++decoded_idx;
            }
            if (decoded_idx == m_log_events.size()
                || m_log_events[decoded_idx].get_log_event_idx() != global_idx)
            {
                return SfaErrorCode{SfaErrorCodeEnum::LogEventIndexUnavailable};
            }
            matching_indices.emplace_back(decoded_idx);
        }
        return matching_indices;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to search archive: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while searching archive: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::SearchFailure};
    }
}

auto ClpArchiveReader::internal_decode_all() -> Result<void> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }

    try {
        prepare_for_decode();
        m_log_events.clear();
        m_log_events.reserve(get_active_event_count());

        std::string message;
        int64_t timestamp{0};
        int64_t log_event_idx{0};
        while (true) {
            std::shared_ptr<clp_s::SchemaReader> next_table{nullptr};
            int64_t next_idx{0};

            for (auto const& table : m_tables) {
                if (nullptr == table) {
                    SPDLOG_ERROR("Failed to decode archive: encountered null schema table.");
                    return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
                }
                if (table->done()) {
                    continue;
                }
                auto const current_idx{table->get_next_log_event_idx()};
                if (nullptr == next_table || current_idx < next_idx) {
                    next_table = table;
                    next_idx = current_idx;
                }
            }

            if (nullptr == next_table) {
                break;
            }

            if (m_selected_file_info.has_value()
                && next_idx >= m_selected_file_info->get_end_index())
            {
                break;
            }

            if (next_table->get_next_message_with_metadata(message, timestamp, log_event_idx)) {
                if (m_selected_file_info.has_value()
                    && log_event_idx < m_selected_file_info->get_start_index())
                {
                    continue;
                }
                m_log_events.emplace_back(log_event_idx, timestamp, std::move(message));
            }
        }
        return ystdlib::error_handling::success();
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to decode archive: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while decoding archive: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

auto ClpArchiveReader::prepare_for_decode() -> void {
    if (m_is_decode_prepared) {
        return;
    }

    m_archive_reader->read_dictionaries_and_metadata();
    m_archive_reader->open_packed_streams();
    m_tables = m_archive_reader->read_all_tables();
    m_is_decode_prepared = true;
}

auto ClpArchiveReader::precompute_archive_metadata() -> Result<void> {
    m_uncompressed_size = m_archive_reader->get_header().uncompressed_size;
    auto const& range_index{m_archive_reader->get_range_index()};
    m_file_names.reserve(range_index.size());
    m_file_infos.reserve(range_index.size());

    for (auto const& range : range_index) {
        auto const start_idx{static_cast<int64_t>(range.start_index)};
        auto const end_idx{static_cast<int64_t>(range.end_index)};
        m_event_count += static_cast<uint64_t>(end_idx - start_idx);

        auto const filename_it{
                range.fields.find(std::string{clp_s::constants::range_index::cFilename})
        };
        auto const filename{filename_it->get<std::string>()};

        m_file_names.push_back(filename);
        m_file_infos.emplace_back(filename, start_idx, end_idx);
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp_s::ffi::sfa

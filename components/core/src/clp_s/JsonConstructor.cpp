#include "JsonConstructor.hpp"

#include <filesystem>
#include <queue>
#include <system_error>

#include <fmt/core.h>
#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include "archive_constants.hpp"
#include "ErrorCode.hpp"
#include "ReaderUtils.hpp"
#include "SchemaTree.hpp"
#include "TraceableException.hpp"

namespace clp_s {
JsonConstructor::JsonConstructor(JsonConstructorOption const& option) : m_option{option} {
    std::error_code error_code;
    if (false == std::filesystem::create_directory(option.output_dir, error_code) && error_code) {
        throw OperationFailed(
                ErrorCodeFailure,
                __FILENAME__,
                __LINE__,
                fmt::format(
                        "Cannot create directory '{}' - {}",
                        option.output_dir,
                        error_code.message()
                )
        );
    }
}

void JsonConstructor::store() {
    m_archive_reader = std::make_unique<ArchiveReader>();
    m_archive_reader->open(m_option.archive_path, m_option.network_auth);
    m_archive_reader->read_dictionaries_and_metadata();

    if (m_option.ordered && false == m_archive_reader->has_log_order()) {
        SPDLOG_WARN("This archive is missing ordering information and can not be decompressed in "
                    "log order. Falling back to out of order decompression.");
    }

    m_archive_reader->open_packed_streams();
    if (false == m_option.ordered || false == m_archive_reader->has_log_order()) {
        FileWriter writer;
        writer.open(
                m_option.output_dir + "/original",
                FileWriter::OpenMode::CreateIfNonexistentForAppending
        );
        m_archive_reader->store(writer);

        writer.close();
    } else {
        construct_in_order();
    }
    m_archive_reader->close();
}

void JsonConstructor::construct_in_order() {
    std::string buffer;
    auto tables = m_archive_reader->read_all_tables();
    using ReaderPointer = std::shared_ptr<SchemaReader>;
    auto cmp = [](ReaderPointer& left, ReaderPointer& right) {
        return left->get_next_log_event_idx() > right->get_next_log_event_idx();
    };
    std::priority_queue record_queue(tables.begin(), tables.end(), cmp);
    // Clear tables vector so that memory gets deallocated after we have marshalled all records for
    // a given table
    tables.clear();

    int64_t first_idx{};
    int64_t last_idx{};
    size_t chunk_size{};
    auto src_path = std::filesystem::path(m_option.output_dir) / m_archive_reader->get_archive_id();
    FileWriter writer;
    writer.open(src_path, FileWriter::OpenMode::CreateForWriting);

    mongocxx::client client;
    mongocxx::collection collection;

    if (m_option.metadata_db.has_value()) {
        try {
            auto const mongo_uri{mongocxx::uri(m_option.metadata_db->mongodb_uri)};
            client = mongocxx::client{mongo_uri};
            collection = client[mongo_uri.database()][m_option.metadata_db->mongodb_collection];
        } catch (mongocxx::exception const& e) {
            throw OperationFailed(ErrorCodeBadParamDbUri, __FILE__, __LINE__, e.what());
        }
    }

    std::vector<bsoncxx::document::value> results;
    auto finalize_chunk = [&](bool open_new_writer) {
        // Add one to last_idx to match clp's behaviour of having the end index be exclusive
        ++last_idx;
        writer.close();
        std::string new_file_name = src_path.string() + "_" + std::to_string(first_idx) + "_"
                                    + std::to_string(last_idx) + ".jsonl";
        auto new_file_path = std::filesystem::path(new_file_name);
        std::error_code ec;
        std::filesystem::rename(src_path, new_file_path, ec);
        if (ec) {
            throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__, ec.message());
        }

        if (m_option.metadata_db.has_value()) {
            results.emplace_back(std::move(bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::decompression::cPath,
                            new_file_path.filename()
                    ),
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::decompression::cStreamId,
                            std::string{m_archive_reader->get_archive_id()}
                    ),
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::decompression::cBeginMsgIx,
                            first_idx
                    ),
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::decompression::cEndMsgIx,
                            last_idx
                    ),
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::decompression::cIsLastChunk,
                            false == open_new_writer
                    )
            )));
        }

        if (m_option.print_ordered_chunk_stats) {
            nlohmann::json json_msg;
            json_msg["path"] = new_file_path.string();
            std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore)
                      << std::endl;
        }

        if (open_new_writer) {
            writer.open(src_path, FileWriter::OpenMode::CreateForWriting);
        }
    };

    while (false == record_queue.empty()) {
        ReaderPointer next = record_queue.top();
        record_queue.pop();
        last_idx = next->get_next_log_event_idx();
        if (0 == chunk_size) {
            first_idx = last_idx;
        }
        next->get_next_message(buffer);
        if (false == next->done()) {
            record_queue.emplace(std::move(next));
        }
        writer.write(buffer.c_str(), buffer.length());
        chunk_size += buffer.length();

        if (0 != m_option.target_ordered_chunk_size
            && chunk_size >= m_option.target_ordered_chunk_size)
        {
            finalize_chunk(true);
            chunk_size = 0;
        }
    }

    if (chunk_size > 0) {
        finalize_chunk(false);
    } else {
        writer.close();
        std::error_code ec;
        std::filesystem::remove(src_path, ec);
        if (ec) {
            throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__, ec.message());
        }
    }

    if (false == results.empty()) {
        try {
            collection.insert_many(results);
        } catch (mongocxx::exception const& e) {
            throw OperationFailed(ErrorCodeFailureDbBulkWrite, __FILE__, __LINE__, e.what());
        }
    }
}
}  // namespace clp_s

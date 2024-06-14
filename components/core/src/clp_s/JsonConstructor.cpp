#include "JsonConstructor.hpp"

#include <filesystem>
#include <queue>
#include <system_error>

#include <fmt/core.h>

#include "ErrorCode.hpp"
#include "ReaderUtils.hpp"
#include "SchemaTree.hpp"
#include "TraceableException.hpp"

namespace clp_s {
JsonConstructor::JsonConstructor(JsonConstructorOption const& option)
        : m_output_dir(option.output_dir),
          m_archives_dir(option.archives_dir),
          m_ordered{option.ordered},
          m_archive_id(option.archive_id) {
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

    std::filesystem::path archive_path{m_archives_dir};
    archive_path /= m_archive_id;
    if (false == std::filesystem::is_directory(archive_path)) {
        throw OperationFailed(
                ErrorCodeFailure,
                __FILENAME__,
                __LINE__,
                fmt::format("'{}' is not a directory", archive_path.c_str())
        );
    }
}

void JsonConstructor::store() {
    FileWriter writer;
    // TODO: change this when doing chunking
    writer.open(m_output_dir + "/original", FileWriter::OpenMode::CreateIfNonexistentForAppending);

    m_archive_reader = std::make_unique<ArchiveReader>();
    m_archive_reader->open(m_archives_dir, m_archive_id);
    m_archive_reader->read_dictionaries_and_metadata();
    if (false == m_ordered) {
        m_archive_reader->store(writer);
    } else {
        construct_in_order(writer);
    }
    m_archive_reader->close();

    writer.close();
}

void JsonConstructor::construct_in_order(FileWriter& writer) {
    std::string buffer;
    auto tables = m_archive_reader->read_all_tables();
    using ReaderPointer = std::shared_ptr<SchemaReader>;
    auto cmp = [](ReaderPointer& left, ReaderPointer& right) {
        return left->get_next_timestamp() > right->get_next_timestamp();
    };
    std::priority_queue record_queue(tables.begin(), tables.end(), cmp);
    // Clear tables vector so that memory gets deallocated after we have marshalled all records for
    // a given table
    tables.clear();
    while (false == record_queue.empty()) {
        ReaderPointer next = record_queue.top();
        record_queue.pop();
        next->get_next_message(buffer);
        if (false == next->done()) {
            record_queue.emplace(std::move(next));
        }
        writer.write(buffer.c_str(), buffer.length());
    }
    writer.close();
}
}  // namespace clp_s

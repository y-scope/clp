#include "JsonConstructor.hpp"

#include <filesystem>
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
          m_current_archive_index(0),
          m_max_archive_index(0) {
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

    if (false == std::filesystem::is_directory(m_archives_dir)) {
        throw OperationFailed(
                ErrorCodeFailure,
                __FILENAME__,
                __LINE__,
                fmt::format("'{}' is not a directory", m_archives_dir)
        );
    }

    for (auto const& entry : std::filesystem::directory_iterator(m_archives_dir)) {
        if (false == entry.is_directory()) {
            // Skip non-directories
            continue;
        }

        m_archive_paths.push_back(entry.path().string());
    }

    if (m_archive_paths.empty()) {
        throw OperationFailed(
                ErrorCodeFailure,
                __FILENAME__,
                __LINE__,
                fmt::format("No sub-archives in '{}'", m_archives_dir)
        );
    }

    m_max_archive_index = m_archive_paths.size() - 1;
}

void JsonConstructor::construct() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    m_schema_tree = ReaderUtils::read_schema_tree(m_archives_dir);
    auto id_to_schema = ReaderUtils::read_schemas(m_archives_dir);

    auto timestamp_dict = ReaderUtils::read_timestamp_dictionary(m_archives_dir);

    m_archive_reader
            = std::make_unique<ArchiveReader>(m_schema_tree, *id_to_schema, timestamp_dict);
}

void JsonConstructor::store() {
    FileWriter writer;
    writer.open(m_output_dir + "/original", FileWriter::OpenMode::CreateIfNonexistentForAppending);

    while (m_current_archive_index <= m_max_archive_index) {
        ArchiveReaderOption option;
        option.archive_path = m_archive_paths[m_current_archive_index];
        m_archive_reader->open(option);
        m_archive_reader->store(writer);
        m_archive_reader->close();
        m_current_archive_index++;
    }

    writer.close();
}

void JsonConstructor::close() {
    //    archive_reader_->Close();
}
}  // namespace clp_s

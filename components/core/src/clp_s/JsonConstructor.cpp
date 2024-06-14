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
    writer.open(m_output_dir + "/original", FileWriter::OpenMode::CreateIfNonexistentForAppending);

    m_archive_reader = std::make_unique<ArchiveReader>();
    m_archive_reader->open(m_archives_dir, m_archive_id);
    m_archive_reader->read_dictionaries_and_metadata();
    m_archive_reader->store(writer);
    m_archive_reader->close();

    writer.close();
}

}  // namespace clp_s

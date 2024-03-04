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
          m_archives_dir(option.archives_dir){
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
}

void JsonConstructor::construct() {
    m_archive_reader = std::make_unique<ArchiveReader>();
}

void JsonConstructor::store() {
    FileWriter writer;
    writer.open(m_output_dir + "/original", FileWriter::OpenMode::CreateIfNonexistentForAppending);

    m_archive_reader->open(m_archives_dir);
    m_archive_reader->read_dictionaries_and_metadata();
    m_archive_reader->store(writer);
    m_archive_reader->close();

    writer.close();
}

}  // namespace clp_s

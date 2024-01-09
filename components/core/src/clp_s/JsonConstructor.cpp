#include "JsonConstructor.hpp"

#include <boost/filesystem.hpp>

#include "ReaderUtils.hpp"
#include "SchemaTree.hpp"

namespace clp_s {
JsonConstructor::JsonConstructor(JsonConstructorOption const& option)
        : m_output_dir(option.output_dir),
          m_archives_dir(option.archives_dir),
          m_current_archive_index(0),
          m_max_archive_index(0) {
    if (false == boost::filesystem::create_directory(m_output_dir)) {
        SPDLOG_ERROR("Can not create directory '{}'", m_output_dir);
        exit(1);
    }

    if (false == boost::filesystem::is_directory(m_archives_dir)) {
        SPDLOG_ERROR("'{}' is not a directory", m_archives_dir);
        exit(1);
    }

    boost::filesystem::directory_iterator iter(m_archives_dir);
    boost::filesystem::directory_iterator end;

    for (; iter != end; ++iter) {
        if (boost::filesystem::is_directory(iter->path())) {
            m_archive_paths.push_back(iter->path().string());
        }
    }

    if (m_archive_paths.empty()) {
        SPDLOG_ERROR("No archives in '{}'", m_archives_dir);
        exit(1);
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
    writer.open(m_output_dir + "/original", FileWriter::OpenMode::CreateForWriting);

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

#include "ArchiveWriter.hpp"

#include <json/single_include/nlohmann/json.hpp>

#include "archive_constants.hpp"
#include "Defs.hpp"
#include "SchemaTree.hpp"

namespace clp_s {
void ArchiveWriter::open(ArchiveWriterOption const& option) {
    m_id = boost::uuids::to_string(option.id);
    m_compression_level = option.compression_level;
    m_print_archive_stats = option.print_archive_stats;
    m_single_file_archive = option.single_file_archive;
    auto archive_path = boost::filesystem::path(option.archives_dir) / m_id;

    boost::system::error_code boost_error_code;
    bool path_exists = boost::filesystem::exists(archive_path, boost_error_code);
    if (path_exists) {
        SPDLOG_ERROR("Archive path already exists: {}", archive_path.c_str());
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    m_archive_path = archive_path.string();
    if (false == boost::filesystem::create_directory(m_archive_path)) {
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    std::string var_dict_path = m_archive_path + constants::cArchiveVarDictFile;
    m_var_dict = std::make_shared<VariableDictionaryWriter>();
    m_var_dict->open(var_dict_path, m_compression_level, UINT64_MAX);

    std::string log_dict_path = m_archive_path + constants::cArchiveLogDictFile;
    m_log_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_log_dict->open(log_dict_path, m_compression_level, UINT64_MAX);

    std::string array_dict_path = m_archive_path + constants::cArchiveArrayDictFile;
    m_array_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_array_dict->open(array_dict_path, m_compression_level, UINT64_MAX);

    std::string timestamp_dict_path = m_archive_path + constants::cArchiveTimestampDictFile;
    m_timestamp_dict = std::make_shared<TimestampDictionaryWriter>();
    m_timestamp_dict->open(timestamp_dict_path, m_compression_level);
}

void ArchiveWriter::close() {
    auto var_dict_compressed_size = m_var_dict->close();
    auto log_dict_compressed_size = m_log_dict->close();
    auto array_dict_compressed_size = m_array_dict->close();
    auto timestamp_dict_compressed_size = m_timestamp_dict->close();
    auto schema_tree_compressed_size = m_schema_tree.store(m_archive_path, m_compression_level);
    auto schema_map_compressed_size = m_schema_map.store(m_archive_path, m_compression_level);
    auto [table_metadata_compressed_size, table_compressed_size] = store_tables();

    if (m_single_file_archive) {
        std::vector<ArchiveFileInfo> files{
                {constants::cArchiveSchemaTreeFile, schema_tree_compressed_size},
                {constants::cArchiveSchemaMapFile, schema_map_compressed_size},
                {constants::cArchiveVarDictFile, var_dict_compressed_size},
                {constants::cArchiveLogDictFile, log_dict_compressed_size},
                {constants::cArchiveArrayDictFile, array_dict_compressed_size},
                {constants::cArchiveTableMetadataFile, table_metadata_compressed_size},
                {constants::cArchiveTablesFile, table_compressed_size}
        };
        uint64_t offset = 0;
        for (auto& file : files) {
            uint64_t original_size = file.o;
            file.o = offset;
            offset += original_size;
        }
        write_single_file_archive(files, timestamp_dict_compressed_size);
    } else {
        m_compressed_size = var_dict_compressed_size + log_dict_compressed_size
                            + array_dict_compressed_size + timestamp_dict_compressed_size
                            + schema_tree_compressed_size + schema_map_compressed_size
                            + table_metadata_compressed_size + table_compressed_size;
    }

    if (m_metadata_db) {
        update_metadata_db();
    }

    if (m_print_archive_stats) {
        print_archive_stats();
    }

    m_id_to_schema_writer.clear();
    m_schema_tree.clear();
    m_schema_map.clear();
    m_encoded_message_size = 0UL;
    m_uncompressed_size = 0UL;
    m_compressed_size = 0UL;
}

void ArchiveWriter::write_single_file_archive(
        std::vector<ArchiveFileInfo> const& files,
        size_t timestamp_dict_compressed_size
) {
    std::string archive_path = m_archive_path + constants::cArchiveFile;
    FileWriter archive_writer;
    archive_writer.open(archive_path, FileWriter::OpenMode::CreateForWriting);

    write_archive_metadata(archive_writer, files, timestamp_dict_compressed_size);
    size_t metadata_section_size = archive_writer.get_pos() - sizeof(ArchiveHeader);
    write_archive_files(archive_writer, files);
    m_compressed_size = archive_writer.get_pos();
    write_archive_header(archive_writer, metadata_section_size);

    archive_writer.close();
}

void ArchiveWriter::write_archive_metadata(
        FileWriter& archive_writer,
        std::vector<ArchiveFileInfo> const& files,
        size_t timestamp_dict_compressed_size
) {
    archive_writer.seek_from_begin(sizeof(ArchiveHeader));

    ZstdCompressor compressor;
    compressor.open(archive_writer, m_compression_level);
    compressor.write_numeric_value(3);  // Number of packets

    // Write archive info
    ArchiveInfoPacket archive_info{.num_segments = 1};
    std::stringstream msgpack_buffer;
    msgpack::pack(msgpack_buffer, archive_info);
    std::string archive_info_str = msgpack_buffer.str();
    compressor.write_numeric_value(ArchiveMetadataPacketType::ArchiveInfo);
    compressor.write_numeric_value(archive_info_str.size());
    compressor.write_string(archive_info_str);

    // Write archive file info
    ArchiveFileInfoPacket archive_file_info{.files{files}};
    msgpack_buffer.clear();
    msgpack::pack(msgpack_buffer, archive_file_info);
    std::string archive_file_info_str = msgpack_buffer.str();
    compressor.write_numeric_value(ArchiveMetadataPacketType::ArchiveFileInfo);
    compressor.write_numeric_value(archive_file_info_str.size());
    compressor.write_string(archive_file_info_str);

    // Write timestamp dictionary
    compressor.write_numeric_value(ArchiveMetadataPacketType::TimestampDictionary);
    compressor.write_numeric_value(timestamp_dict_compressed_size);
    std::string timestamp_dict_path = m_archive_path + constants::cArchiveTimestampDictFile;
    FileReader timestamp_dict_reader;
    timestamp_dict_reader.open(timestamp_dict_path);
    char read_buffer[cReadBlockSize];
    while (true) {
        size_t num_bytes_read{0};
        ErrorCode error_code
                = timestamp_dict_reader.try_read(read_buffer, cReadBlockSize, num_bytes_read);
        if (ErrorCodeSuccess != error_code) {
            break;
        }
        compressor.write(read_buffer, num_bytes_read);
    }
    timestamp_dict_reader.close();
    std::filesystem::remove(timestamp_dict_path);
    compressor.close();
}

void ArchiveWriter::write_archive_files(
        FileWriter& archive_writer,
        std::vector<ArchiveFileInfo> const& files
) {
    for (auto const& file : files) {
        std::string file_path = m_archive_path + file.n;
        FileReader reader;
        reader.open(file_path);
        char read_buffer[cReadBlockSize];
        while (true) {
            size_t num_bytes_read{0};
            ErrorCode const error_code
                    = reader.try_read(read_buffer, cReadBlockSize, num_bytes_read);
            if (ErrorCodeSuccess != error_code) {
                break;
            }
            archive_writer.write(read_buffer, num_bytes_read);
        }
        reader.close();
        boost::filesystem::remove(file_path);
    }
}

void ArchiveWriter::write_archive_header(FileWriter& archive_writer, size_t metadata_section_size) {
    ArchiveHeader header{
            .magic_number{0},
            .version
            = (cArchiveMajorVersion << 24) | (cArchiveMinorVersion << 16) | cArchivePatchVersion,
            .uncompressed_size = m_uncompressed_size,
            .compressed_size = m_compressed_size,
            .reserved_padding{0},
            .metadata_section_size = static_cast<uint32_t>(metadata_section_size),
            .compression_type = static_cast<uint16_t>(ArchiveCompressionType::Zstd),
            .padding = 0
    };
    std::memcpy(&header.magic_number, "ARCHIVES", sizeof(header.magic_number));
    archive_writer.seek_from_begin(0);
    archive_writer.write(reinterpret_cast<char const*>(&header), sizeof(header));
}

void ArchiveWriter::append_message(
        int32_t schema_id,
        Schema const& schema,
        ParsedMessage& message
) {
    SchemaWriter* schema_writer;
    auto it = m_id_to_schema_writer.find(schema_id);
    if (it != m_id_to_schema_writer.end()) {
        schema_writer = it->second;
    } else {
        schema_writer = new SchemaWriter();
        initialize_schema_writer(schema_writer, schema);
        m_id_to_schema_writer[schema_id] = schema_writer;
    }

    m_encoded_message_size += schema_writer->append_message(message);
}

size_t ArchiveWriter::get_data_size() {
    return m_log_dict->get_data_size() + m_var_dict->get_data_size() + m_array_dict->get_data_size()
           + m_encoded_message_size;
}

void ArchiveWriter::initialize_schema_writer(SchemaWriter* writer, Schema const& schema) {
    for (int32_t id : schema) {
        if (Schema::schema_entry_is_unordered_object(id)) {
            continue;
        }
        auto const& node = m_schema_tree.get_node(id);
        switch (node.get_type()) {
            case NodeType::Integer:
                writer->append_column(new Int64ColumnWriter(id));
                break;
            case NodeType::Float:
                writer->append_column(new FloatColumnWriter(id));
                break;
            case NodeType::ClpString:
                writer->append_column(new ClpStringColumnWriter(id, m_var_dict, m_log_dict));
                break;
            case NodeType::VarString:
                writer->append_column(new VariableStringColumnWriter(id, m_var_dict));
                break;
            case NodeType::Boolean:
                writer->append_column(new BooleanColumnWriter(id));
                break;
            case NodeType::UnstructuredArray:
                writer->append_column(new ClpStringColumnWriter(id, m_var_dict, m_array_dict));
                break;
            case NodeType::DateString:
                writer->append_column(new DateStringColumnWriter(id));
                break;
            case NodeType::StructuredArray:
            case NodeType::Object:
            case NodeType::NullValue:
            case NodeType::Unknown:
                break;
        }
    }
}

std::pair<size_t, size_t> ArchiveWriter::store_tables() {
    m_tables_file_writer.open(
            m_archive_path + constants::cArchiveTablesFile,
            FileWriter::OpenMode::CreateForWriting
    );
    m_table_metadata_file_writer.open(
            m_archive_path + constants::cArchiveTableMetadataFile,
            FileWriter::OpenMode::CreateForWriting
    );
    m_table_metadata_compressor.open(m_table_metadata_file_writer, m_compression_level);
    m_table_metadata_compressor.write_numeric_value(m_id_to_schema_writer.size());
    for (auto& i : m_id_to_schema_writer) {
        m_table_metadata_compressor.write_numeric_value(i.first);
        m_table_metadata_compressor.write_numeric_value(i.second->get_num_messages());
        m_table_metadata_compressor.write_numeric_value(m_tables_file_writer.get_pos());

        m_tables_compressor.open(m_tables_file_writer, m_compression_level);
        size_t uncompressed_size = i.second->store(m_tables_compressor);
        m_tables_compressor.close();
        delete i.second;

        m_table_metadata_compressor.write_numeric_value(uncompressed_size);
    }
    m_table_metadata_compressor.close();

    auto table_metadata_compressed_size = m_table_metadata_file_writer.get_pos();
    auto table_compressed_size = m_tables_file_writer.get_pos();

    m_table_metadata_file_writer.close();
    m_tables_file_writer.close();

    return {table_metadata_compressed_size, table_compressed_size};
}

void ArchiveWriter::update_metadata_db() {
    m_metadata_db->open();
    clp::streaming_archive::ArchiveMetadata metadata(
            cArchiveFormatDevelopmentVersionFlag,
            "",
            0ULL
    );
    metadata.increment_static_compressed_size(m_compressed_size);
    metadata.increment_static_uncompressed_size(m_uncompressed_size);
    metadata.expand_time_range(
            m_timestamp_dict->get_begin_timestamp(),
            m_timestamp_dict->get_end_timestamp()
    );

    m_metadata_db->add_archive(m_id, metadata);
    m_metadata_db->close();
}

void ArchiveWriter::print_archive_stats() {
    nlohmann::json json_msg;
    json_msg["id"] = m_id;
    json_msg["uncompressed_size"] = m_uncompressed_size;
    json_msg["size"] = m_compressed_size;
    std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore) << std::endl;
}
}  // namespace clp_s

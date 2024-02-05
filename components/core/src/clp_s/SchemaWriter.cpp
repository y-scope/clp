#include "SchemaWriter.hpp"

#include <utility>

namespace clp_s {
void SchemaWriter::open(std::string path, int compression_level) {
    m_path = std::move(path);
    m_compression_level = compression_level;
}

size_t SchemaWriter::close() {
    m_compressor.close();
    size_t compressed_size = m_file_writer.get_pos();
    m_file_writer.close();

    for (auto i : m_columns) {
        delete i;
    }

    m_columns.clear();
    return compressed_size;
}

void SchemaWriter::append_column(BaseColumnWriter* column_writer) {
    m_columns.push_back(column_writer);
}

size_t SchemaWriter::append_message(ParsedMessage& message) {
    int count = 0;
    size_t size, total_size;
    size = total_size = 0;
    for (auto& i : message.get_content()) {
        m_columns[count]->add_value(i.second, size);
        total_size += size;
        count++;
    }

    m_num_messages++;
    return total_size;
}

void SchemaWriter::store() {
    m_file_writer.open(m_path, FileWriter::OpenMode::CreateForWriting);
    m_file_writer.write_numeric_value(m_num_messages);
    m_compressor.open(m_file_writer, m_compression_level);

    for (auto& writer : m_columns) {
        writer->store(m_compressor);
        //        compressor_.Write(writer->GetData(), writer->GetSize());
    }
}

SchemaWriter::~SchemaWriter() {
    for (auto i : m_columns) {
        delete i;
    }
}
}  // namespace clp_s

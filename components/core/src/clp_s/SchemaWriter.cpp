#include "SchemaWriter.hpp"

#include <utility>

namespace clp_s {
void SchemaWriter::append_column(BaseColumnWriter* column_writer) {
    m_total_uncompressed_size += column_writer->get_total_header_size();
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

    for (auto& i : message.get_unordered_content()) {
        m_columns[count]->add_value(i, size);
        total_size += size;
        ++count;
    }

    m_num_messages++;
    m_total_uncompressed_size += total_size;
    return total_size;
}

void SchemaWriter::store(ZstdCompressor& compressor) {
    for (auto& writer : m_columns) {
        writer->store(compressor);
    }
}

SchemaWriter::~SchemaWriter() {
    for (auto i : m_columns) {
        delete i;
    }
}
}  // namespace clp_s

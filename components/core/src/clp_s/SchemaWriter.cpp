#include "SchemaWriter.hpp"

#include <utility>

namespace clp_s {
void SchemaWriter::append_column(std::unique_ptr<BaseColumnWriter> column_writer) {
    m_total_uncompressed_size += column_writer->get_total_header_size();
    m_columns.emplace_back(std::move(column_writer));
}

size_t SchemaWriter::append_message(ParsedMessage& message) {
    int count{};
    size_t total_size{};
    for (auto& i : message.get_content()) {
        total_size += m_columns[count]->add_value(i.second);
        ++count;
    }

    for (auto& i : message.get_unordered_content()) {
        total_size += m_columns[count]->add_value(i);
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
}  // namespace clp_s

#include "SchemaMap.hpp"

#include "archive_constants.hpp"
#include "FileWriter.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
int32_t SchemaMap::add_schema(Schema const& schema) {
    auto const schema_it = m_schema_map.find(schema);
    if (m_schema_map.end() != schema_it) {
        return schema_it->second;
    }
    m_schema_map.emplace(schema, m_current_schema_id);
    return m_current_schema_id++;
}

size_t SchemaMap::store(std::string const& archives_dir, int compression_level) {
    FileWriter schema_map_writer;
    ZstdCompressor schema_map_compressor;

    // TODO: rename schema_ids -> schema_map, and use int32_t for schema size
    schema_map_writer.open(
            archives_dir + constants::cArchiveSchemaMapFile,
            FileWriter::OpenMode::CreateForWriting
    );
    schema_map_compressor.open(schema_map_writer, compression_level);
    schema_map_compressor.write_numeric_value(m_schema_map.size());
    for (auto const& schema_mapping : m_schema_map) {
        auto const& schema = schema_mapping.first;
        schema_map_compressor.write_numeric_value(schema_mapping.second);
        schema_map_compressor.write_numeric_value(static_cast<uint32_t>(schema.size()));
        schema_map_compressor.write_numeric_value(static_cast<uint32_t>(schema.get_num_ordered()));
        for (int32_t mst_node_id : schema) {
            schema_map_compressor.write_numeric_value(mst_node_id);
        }
    }

    schema_map_compressor.close();
    size_t compressed_size = schema_map_writer.get_pos();
    schema_map_writer.close();
    return compressed_size;
}
}  // namespace clp_s

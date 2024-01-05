#include "SchemaMap.hpp"

#include "FileWriter.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
int32_t SchemaMap::add_schema(std::set<int32_t>& schema) {
    auto schema_it = m_schema_map.find(schema);
    if (schema_it != m_schema_map.end()) {
        return schema_it->second;
    } else {
        m_schema_map[schema] = m_current_schema_id;
        return m_current_schema_id++;
    }
}

void SchemaMap::mark_used(int32_t schema_id) {
    m_used_schema_ids.insert(schema_id);
}

void SchemaMap::store() {
    FileWriter schema_map_writer;
    ZstdCompressor schema_map_compressor;

    // TODO: rename schema_ids -> schema_map, and use int32_t for schema size
    schema_map_writer.open(m_archive_dir + "/schema_ids", FileWriter::OpenMode::CreateForWriting);
    schema_map_compressor.open(schema_map_writer, m_compression_level);
    schema_map_compressor.write_numeric_value(get_num_used_schemas());
    for (auto const& schema_mapping : m_schema_map) {
        auto const& schema = schema_mapping.first;
        if (m_used_schema_ids.count(schema_mapping.second)) {
            schema_map_compressor.write_numeric_value(schema_mapping.second);
            schema_map_compressor.write_numeric_value(schema.size());
            for (int32_t mst_node_id : schema) {
                schema_map_compressor.write_numeric_value(mst_node_id);
            }
        }
    }

    schema_map_compressor.close();
    schema_map_writer.close();
}
}  // namespace clp_s

#ifndef CLP_S_SCHEMAMAP_HPP
#define CLP_S_SCHEMAMAP_HPP

#include <map>
#include <set>
#include <string>

namespace clp_s {
class SchemaMap {
public:
    typedef std::map<std::set<int32_t>, int32_t> schema_map_t;

    // Constructor
    explicit SchemaMap(std::string const& archives_dir, int compression_level)
            : m_archives_dir(archives_dir),
              m_compression_level(compression_level),
              m_current_schema_id(0) {}

    /**
     * Return a schema's Id and add the schema to the
     * schema map if it does not already exist.
     * @param schema
     * @return the Id of the schema
     */
    int32_t add_schema(std::set<int32_t>& schema);

    /**
     * Write the contents of the SchemaMap to archives_dir/schema_ids
     */
    void store();

    /**
     * Get const iterators into the schema map
     * @return const it to the schema map
     */
    schema_map_t::const_iterator schema_map_begin() const { return m_schema_map.cbegin(); }

    schema_map_t::const_iterator schema_map_end() const { return m_schema_map.cend(); }

private:
    std::string m_archives_dir;
    int m_compression_level;
    int32_t m_current_schema_id;
    schema_map_t m_schema_map;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAMAP_HPP

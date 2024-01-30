#ifndef CLP_S_SCHEMA_HPP
#define CLP_S_SCHEMA_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace clp_s {
/**
 * Class representing a schema made up of MST nodes.
 */
class Schema {
public:
    using schema_t = std::vector<int32_t>;

    /**
     * Inserts a node into the ordered region of the schema.
     */
    void insert_ordered(int32_t mst_node_id);

    /**
     * Inserts a node into the unordered region of the schema.
     */
    void insert_unordered(int32_t mst_node_id);

    /**
     * Inserts an unordered list of nodes into the unordered region of the schema.
     */
    void insert_unordered(schema_t const& mst_node_ids);

    /**
     * Clear the Schema object so that it can be reused without reallocating the underlying vector.
     */
    void clear() {
        m_schema.clear();
        m_num_unordered = 0;
    }

    /**
     * @return the number of elements in the underlying schema
     */
    [[nodiscard]] size_t size() const { return m_schema.size(); }

    /**
     * @return iterators to the underlying schema
     */
    auto begin() { return m_schema.begin(); }

    auto end() { return m_schema.end(); }

    /**
     * @return constant iterators to the underlying schema
     */
    auto begin() const { return m_schema.cbegin(); }

    auto end() const { return m_schema.cend(); }

    /**
     * Comparison operators so that Schema can act as a key for SchemaMap
     */
    bool operator<(Schema const& rhs) const { return m_schema < rhs.m_schema; }

    bool operator==(Schema const& rhs) const { return m_schema == rhs.m_schema; }

private:
    std::vector<int32_t> m_schema;
    size_t m_num_unordered{0};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMA_HPP

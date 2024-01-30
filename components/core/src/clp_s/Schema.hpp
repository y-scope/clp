#ifndef CLP_S_SCHEMA_HPP
#define CLP_S_SCHEMA_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace clp_s {
/**
 * Class representing a schema made up of MST nodes.
 *
 * Internally, the schema is represented by a vector where the first m_num_ordered entries are
 * ordered by MST node ID, and the following entries are allowed to have arbitrary order.
 *
 * In the current implementation of clp-s, MST node IDs in a schema must be unique, so the caller
 * is responsible for not inserting duplicate MST nodes into a schema. Future versions of clp-s will
 * likely relax this requirement.
 */
class Schema {
public:
    /**
     * Inserts a node into the ordered region of the schema.
     */
    void insert_ordered(int32_t mst_node_id);

    /**
     * Inserts a node into the unordered region of the schema.
     */
    void insert_unordered(int32_t mst_node_id);

    /**
     * Inserts another schema into the unordered region of the schema, maintaining that Schema's
     * order.
     */
    void insert_unordered(Schema const& schema);

    /**
     * Clear the Schema object so that it can be reused without reallocating the underlying vector.
     */
    void clear() {
        m_schema.clear();
        m_num_ordered = 0;
    }

    /**
     * @return the number of elements in the underlying schema
     */
    [[nodiscard]] size_t size() const { return m_schema.size(); }

    /**
     * @return iterator to the start of the underlying schema
     */
    [[nodiscard]] auto begin() { return m_schema.begin(); }

    /**
     * @return iterator to the end of the underlying schema
     */
    [[nodiscard]] auto end() { return m_schema.end(); }

    /**
     * @return constant iterator to the start of the underlying schema
     */
    [[nodiscard]] auto begin() const { return m_schema.cbegin(); }

    /**
     * @return constant iterator to the end of the underlying schema
     */
    [[nodiscard]] auto end() const { return m_schema.cend(); }

    /**
     * @return constant iterator to the start of the underlying schema
     */
    [[nodiscard]] auto cbegin() const { return m_schema.cbegin(); }

    /**
     * @return constant iterator to the end of the underlying schema
     */
    [[nodiscard]] auto cend() const { return m_schema.cend(); }

    /**
     * Less than comparison operator so that Schema can act as a key for SchemaMap
     * @return true if this schema is less than the schema on the right hand side
     * @return false otherwise
     */
    bool operator<(Schema const& rhs) const { return m_schema < rhs.m_schema; }

    /**
     * Equal to comparison operator so that Schema can act as a key for SchemaMap
     * @return true if this schema is equal to the schema on the right hand side
     * @return false otherwise
     */
    bool operator==(Schema const& rhs) const { return m_schema == rhs.m_schema; }

private:
    std::vector<int32_t> m_schema;
    size_t m_num_ordered{0};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMA_HPP

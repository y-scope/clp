#ifndef CLP_S_SCHEMA_HPP
#define CLP_S_SCHEMA_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "SchemaTree.hpp"
#include "TraceableException.hpp"
#include "Utils.hpp"

namespace clp_s {
/**
 * Class representing a schema made up of MST nodes.
 *
 * Internally, the schema is represented by a vector where the first m_num_ordered entries are
 * ordered by MST node ID, and the following entries are allowed to have arbitrary order.
 *
 * In the current implementation of clp-s, MST node IDs must be unique in the ordered region of a
 * schema, but can be repeated in the unordered region. The caller is responsible for not inserting
 * duplicate MST nodes into the ordered region of a schema.
 */
class Schema {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

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
     * Sets the number of ordered nodes present in the schema. This method is used during
     * decompression to help initialize this object.
     * @param num_ordered
     */
    void set_num_ordered(size_t num_ordered) { m_num_ordered = num_ordered; }

    /**
     * @return the number of ordered elements in the underlying schema
     */
    [[nodiscard]] size_t get_num_ordered() const { return m_num_ordered; }

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
     * @return the nth value in the underlying schema
     */
    int32_t operator[](size_t i) const { return m_schema[i]; }

    /**
     * @return a view into the ordered region of the underlying schema
     */
    [[nodiscard]] std::span<int32_t> get_ordered_schema_view() {
        return std::span<int32_t>{m_schema.data(), m_num_ordered};
    }

    /**
     * @param i
     * @param size
     * @return a view into the requested region of the schema
     */
    [[nodiscard]] std::span<int32_t> get_view(size_t i, size_t size) {
        if (i + size > m_schema.size()) {
            throw OperationFailed(ErrorCodeOutOfBounds, __FILENAME__, __LINE__);
        }
        return std::span<int32_t>{m_schema.data() + i, size};
    }

    /**
     * Resizes the internal schema vector to match the given length.
     * @param size
     */
    void resize(size_t size) { m_schema.resize(size); }

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

    /**
     * Starts an unordered object of a given NodeType.
     *
     * Unordered objects must be closed by calling the `end_unordered_object` method with the start
     * position returned by this method.
     * @param object_type
     * @return the start position of the unordered object
     */
    [[nodiscard]] size_t start_unordered_object(NodeType object_type) {
        insert_unordered(encode_node_type_as_schema_entry(object_type));
        return m_schema.size();
    }

    /**
     * Ends an unordered object which was started by calling `start_unordered_object`.
     * @param start_position
     */
    void end_unordered_object(size_t start_position) {
        m_schema[start_position - 1] |= static_cast<int32_t>(m_schema.size() - start_position);
    }

    /**
     * Encodes a NodeType as a schema entry to delimit an unordered object using bithacks.
     * @param node_type
     * @return The NodeType encoded as a schema entry
     */
    static int32_t encode_node_type_as_schema_entry(NodeType node_type) {
        return static_cast<int32_t>(node_type) << cEncodedTypeOffset;
    }

    /**
     * Checks if a schema entry is the delimeter for an unordered object.
     * @param schema_entry
     * @return true if the schema_entry is the delimiter for an unordered object, false otherwise
     */
    static bool schema_entry_is_unordered_object(int32_t schema_entry) {
        return 0 != (schema_entry & cEncodedTypeBitmask);
    }

    /**
     * Extracts the NodeType from an unordered object delimiter.
     * @param schema_entry
     * @return The extracted NodeType
     */
    static NodeType get_unordered_object_type(int32_t schema_entry) {
        return static_cast<NodeType>(static_cast<uint32_t>(schema_entry) >> cEncodedTypeOffset);
    }

    /**
     * Extracts the unordered object length from an unordered object delimiter.
     * @param schema_entry
     * @return The extracted object length
     */
    static int32_t get_unordered_object_length(int32_t schema_entry) {
        return schema_entry & cEncodedTypeLengthBitmask;
    }

private:
    static constexpr size_t cEncodedTypeOffset = (sizeof(int32_t) - 1) * 8;
    static constexpr int32_t cEncodedTypeBitmask = 0xFF00'0000;
    static constexpr int32_t cEncodedTypeLengthBitmask = ~cEncodedTypeBitmask;

    std::vector<int32_t> m_schema;
    size_t m_num_ordered{0};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMA_HPP

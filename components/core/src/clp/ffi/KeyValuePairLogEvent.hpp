#ifndef CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP
#define CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include <outcome/single-header/outcome.hpp>

#include "../time_types.hpp"
#include "SchemaTree.hpp"
#include "SchemaTreeNode.hpp"
#include "Value.hpp"

namespace clp::ffi {
/**
 * A log event containing key-value pairs. Each event contains:
 * - A collection of node-ID & value pairs, where each pair represents a leaf `SchemaTreeNode` in
 *   the `SchemaTree`.
 * - A reference to the `SchemaTree`
 * - The UTC offset of the current log event
 */
class KeyValuePairLogEvent {
public:
    // Types
    using NodeIdValuePairs = std::unordered_map<SchemaTreeNode::id_t, std::optional<Value>>;

    // Factory functions
    /**
     * @param schema_tree
     * @param node_id_value_pairs
     * @param utc_offset
     * @return A result containing the key-value pair log event or an error code indicating the
     * failure. See `validate_node_id_value_pairs` for the possible error codes.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<SchemaTree const> schema_tree,
            NodeIdValuePairs node_id_value_pairs,
            UtcOffset utc_offset
    ) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent>;

    // Disable copy constructor and assignment operator
    KeyValuePairLogEvent(KeyValuePairLogEvent const&) = delete;
    auto operator=(KeyValuePairLogEvent const&) -> KeyValuePairLogEvent& = delete;

    // Default move constructor and assignment operator
    KeyValuePairLogEvent(KeyValuePairLogEvent&&) = default;
    auto operator=(KeyValuePairLogEvent&&) -> KeyValuePairLogEvent& = default;

    // Destructor
    ~KeyValuePairLogEvent() = default;

    // Methods
    [[nodiscard]] auto get_schema_tree() const -> SchemaTree const& { return *m_schema_tree; }

    [[nodiscard]] auto get_node_id_value_pairs() const -> NodeIdValuePairs const& {
        return m_node_id_value_pairs;
    }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

private:
    // Constructor
    KeyValuePairLogEvent(
            std::shared_ptr<SchemaTree const> schema_tree,
            NodeIdValuePairs node_id_value_pairs,
            UtcOffset utc_offset
    )
            : m_schema_tree{std::move(schema_tree)},
              m_node_id_value_pairs{std::move(node_id_value_pairs)},
              m_utc_offset{utc_offset} {}

    // Variables
    std::shared_ptr<SchemaTree const> m_schema_tree;
    NodeIdValuePairs m_node_id_value_pairs;
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

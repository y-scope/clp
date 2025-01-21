#ifndef CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP
#define CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../time_types.hpp"
#include "SchemaTree.hpp"
#include "Value.hpp"

namespace clp::ffi {
/**
 * A log event containing key-value pairs. Each event contains:
 * - A reference to the schema tree for auto-generated keys.
 * - A reference to the schema tree for user-generated keys.
 * - A collection of auto-generated node-ID & value pairs, where each pair represents a leaf
 *   `SchemaTree::Node` in the schema tree for auto-generated keys.
 * - A collection of user-generated node-ID & value pairs, where each pair represents a leaf
 *   `SchemaTree::Node` in the schema tree for user-generated keys.
 * - The UTC offset of the current log event.
 */
class KeyValuePairLogEvent {
public:
    // Types
    using NodeIdValuePairs = std::unordered_map<SchemaTree::Node::id_t, std::optional<Value>>;

    // Factory functions
    /**
     * @param auto_gen_keys_schema_tree
     * @param user_gen_keys_schema_tree
     * @param auto_gen_node_id_value_pairs
     * @param user_gen_node_id_value_pairs
     * @param utc_offset
     * @return A result containing the key-value pair log event or an error code indicating the
     * failure:
     * - std::errc::invalid_argument if any of the given schema tree pointers are null.
     * - Forwards `validate_node_id_value_pairs`'s return values.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<SchemaTree const> auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree const> user_gen_keys_schema_tree,
            NodeIdValuePairs auto_gen_node_id_value_pairs,
            NodeIdValuePairs user_gen_node_id_value_pairs,
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
    [[nodiscard]] auto get_auto_gen_keys_schema_tree() const -> SchemaTree const& {
        return *m_auto_gen_keys_schema_tree;
    }

    [[nodiscard]] auto get_user_gen_keys_schema_tree() const -> SchemaTree const& {
        return *m_user_gen_keys_schema_tree;
    }

    [[nodiscard]] auto get_auto_gen_node_id_value_pairs() const -> NodeIdValuePairs const& {
        return m_auto_gen_node_id_value_pairs;
    }

    [[nodiscard]] auto get_user_gen_node_id_value_pairs() const -> NodeIdValuePairs const& {
        return m_user_gen_node_id_value_pairs;
    }

    /**
     * @return A result containing a bitmap where every bit corresponds to the ID of a node in the
     * schema tree for auto-generated keys, and the set bits correspond to the nodes in the subtree
     * defined by all paths from the root node to the nodes in `m_auto_gen_node_id_value_pairs`; or
     * an error code indicating a failure:
     * - Forwards `get_schema_subtree_bitmap`'s return values.
     */
    [[nodiscard]] auto get_auto_gen_keys_schema_subtree_bitmap() const
            -> OUTCOME_V2_NAMESPACE::std_result<std::vector<bool>>;

    /**
     * @return A result containing a bitmap where every bit corresponds to the ID of a node in the
     * schema tree for user-generated keys, and the set bits correspond to the nodes in the subtree
     * defined by all paths from the root node to the nodes in `m_user_gen_node_id_value_pairs`; or
     * an error code indicating a failure:
     * - Forwards `get_schema_subtree_bitmap`'s return values.
     */
    [[nodiscard]] auto get_user_gen_keys_schema_subtree_bitmap() const
            -> OUTCOME_V2_NAMESPACE::std_result<std::vector<bool>>;

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

    /**
     * Serializes the log event into `nlohmann::json` objects.
     * @return A result containing a pair or an error code indicating the failure:
     * - The pair:
     *   - Serialized auto-generated key-value pairs as a JSON object
     *   - Serialized user-generated key-value pairs as a JSON object
     * - The possible error codes:
     *   - Forwards `get_auto_gen_keys_schema_subtree_bitmap`'s return values on failure.
     *   - Forwards `serialize_node_id_value_pairs_to_json`'s return values on failure.
     */
    [[nodiscard]] auto serialize_to_json() const
            -> OUTCOME_V2_NAMESPACE::std_result<std::pair<nlohmann::json, nlohmann::json>>;

private:
    // Constructor
    KeyValuePairLogEvent(
            std::shared_ptr<SchemaTree const> auto_gen_keys_schema_tree,
            std::shared_ptr<SchemaTree const> user_gen_keys_schema_tree,
            NodeIdValuePairs auto_gen_node_id_value_pairs,
            NodeIdValuePairs user_gen_node_id_value_pairs,
            UtcOffset utc_offset
    )
            : m_auto_gen_keys_schema_tree{std::move(auto_gen_keys_schema_tree)},
              m_user_gen_keys_schema_tree{std::move(user_gen_keys_schema_tree)},
              m_auto_gen_node_id_value_pairs{std::move(auto_gen_node_id_value_pairs)},
              m_user_gen_node_id_value_pairs{std::move(user_gen_node_id_value_pairs)},
              m_utc_offset{utc_offset} {}

    // Variables
    std::shared_ptr<SchemaTree const> m_auto_gen_keys_schema_tree;
    std::shared_ptr<SchemaTree const> m_user_gen_keys_schema_tree;
    NodeIdValuePairs m_auto_gen_node_id_value_pairs;
    NodeIdValuePairs m_user_gen_node_id_value_pairs;
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

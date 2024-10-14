#ifndef CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP
#define CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../time_types.hpp"
#include "SchemaTree.hpp"
#include "Value.hpp"

namespace clp::ffi {
/**
 * A log event containing key-value pairs. Each event contains:
 * - A reference to the schema tree of auto-generated keys.
 * - A reference to the schema tree of user-generated keys.
 * - A collection of auto-generated node-ID & value pairs, where each pair represents a leaf
 *   `SchemaTree::Node` in the `SchemaTree`.
 * - A collection of user-generated node-ID & value pairs, where each pair represents a leaf
 *   `SchemaTree::Node` in the `SchemaTree`.
 * - The UTC offset of the current log event.
 */
class KeyValuePairLogEvent {
public:
    // Types
    using NodeIdValuePairs = std::unordered_map<SchemaTree::Node::id_t, std::optional<Value>>;

    // Factory functions
    /**
     * @param auto_generated_schema_tree
     * @param user_generated_schema_tree
     * @param auto_generated_node_id_value_pairs
     * @param user_generated_node_id_value_pairs
     * @param utc_offset
     * @return A result containing the key-value pair log event or an error code indicating the
     * failure, or an error code indicating the failure:
     * - std::errc::invalid_argument if any of the given schema tree pointers are null.
     * - Forwards `validate_node_id_value_pairs`'s return values.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<SchemaTree const> auto_generated_schema_tree,
            std::shared_ptr<SchemaTree const> user_generated_schema_tree,
            NodeIdValuePairs auto_generated_node_id_value_pairs,
            NodeIdValuePairs user_generated_node_id_value_pairs,
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
    [[nodiscard]] auto get_auto_generated_schema_tree() const -> SchemaTree const& {
        return *m_auto_generated_schema_tree;
    }

    [[nodiscard]] auto get_user_generated_schema_tree() const -> SchemaTree const& {
        return *m_user_generated_schema_tree;
    }

    [[nodiscard]] auto get_auto_generated_node_id_value_pairs() const -> NodeIdValuePairs const& {
        return m_auto_generated_node_id_value_pairs;
    }

    [[nodiscard]] auto get_user_generated_node_id_value_pairs() const -> NodeIdValuePairs const& {
        return m_user_generated_node_id_value_pairs;
    }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

    /**
     * Serializes the log event into `nlohmann::json` objects.
     * @return A result containing a pair of serialized JSON objects (the first contains
     * auto-generated key-value pairs, while the second contains user-generated key-value pairs), or
     * an error code indicating the failure:
     * - Forwards `serialize_node_id_value_pairs_to_json`'s return values.
     */
    [[nodiscard]] auto serialize_to_json(
    ) const -> OUTCOME_V2_NAMESPACE::std_result<std::pair<nlohmann::json, nlohmann::json>>;

private:
    // Constructor
    KeyValuePairLogEvent(
            std::shared_ptr<SchemaTree const> auto_generated_schema_tree,
            std::shared_ptr<SchemaTree const> user_generated_schema_tree,
            NodeIdValuePairs auto_generated_node_id_value_pairs,
            NodeIdValuePairs user_generated_node_id_value_pairs,
            UtcOffset utc_offset
    )
            : m_auto_generated_schema_tree{std::move(auto_generated_schema_tree)},
              m_user_generated_schema_tree{std::move(user_generated_schema_tree)},
              m_auto_generated_node_id_value_pairs{std::move(auto_generated_node_id_value_pairs)},
              m_user_generated_node_id_value_pairs{std::move(user_generated_node_id_value_pairs)},
              m_utc_offset{utc_offset} {}

    // Variables
    std::shared_ptr<SchemaTree const> m_auto_generated_schema_tree;
    std::shared_ptr<SchemaTree const> m_user_generated_schema_tree;
    NodeIdValuePairs m_auto_generated_node_id_value_pairs;
    NodeIdValuePairs m_user_generated_node_id_value_pairs;
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

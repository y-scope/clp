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
#include "SchemaTreeNode.hpp"
#include "Value.hpp"

namespace clp::ffi {
/**
 * Class for key-value pair log events. Each key-value pair log event contains the following items:
 *  - A list of key-value pairs
 *  - A reference to the schema tree
 *  - The UTC offset of the current log event
 */
class KeyValuePairLogEvent {
public:
    // Types
    using KeyValuePairs = std::unordered_map<SchemaTreeNode::id_t, std::optional<Value>>;

    // Factory functions
    /**
     * Creates a key-value pair log event from valid given inputs.
     * @param schema_tree
     * @param kv_pairs
     * @param utc_offset
     * @return A result containing the key-value pair log event or an error code indicating the
     * failure:
     */
    [[nodiscard]] static auto
    create(std::shared_ptr<SchemaTree> schema_tree, KeyValuePairs kv_pairs, UtcOffset utc_offset
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

    [[nodiscard]] auto get_key_value_pairs() const -> KeyValuePairs const& { return m_kv_pairs; }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

    [[nodiscard]] auto serialize_to_json(
    ) const -> OUTCOME_V2_NAMESPACE::std_result<nlohmann::json>;

private:
    // Constructor
    KeyValuePairLogEvent(
            std::shared_ptr<SchemaTree> schema_tree,
            KeyValuePairs key_value_pairs,
            UtcOffset utc_offset
    )
            : m_schema_tree{std::move(schema_tree)},
              m_kv_pairs{std::move(key_value_pairs)},
              m_utc_offset{utc_offset} {}

    std::shared_ptr<SchemaTree> m_schema_tree;
    KeyValuePairs m_kv_pairs;
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

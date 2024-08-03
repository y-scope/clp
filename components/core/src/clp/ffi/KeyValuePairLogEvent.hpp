#ifndef CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP
#define CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

#include <memory>
#include <optional>
#include <utility>
#include <vector>

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
    /**
     * Class for an instance of CLP key value pair.
     */
    class KeyValuePair {
    public:
        // Constructors
        /**
         * Constructs a key-value pair with the given key id and value.
         * @param key_id
         * @param value
         */
        explicit KeyValuePair(SchemaTreeNode::id_t key_id, Value value)
                : m_key_id{key_id},
                  m_value{std::move(value)} {}

        /**
         * Constructs a key-value pair with the given key id and a empty value.
         * @param key_id
         */
        explicit KeyValuePair(SchemaTreeNode::id_t key_id)
                : m_key_id{key_id},
                  m_value{std::nullopt} {}

        // Disable copy constructor and assignment operator
        KeyValuePair(KeyValuePair const&) = delete;
        auto operator=(KeyValuePair const&) -> KeyValuePair& = delete;

        // Default move constructor and assignment operator
        KeyValuePair(KeyValuePair&&) = default;
        auto operator=(KeyValuePair&&) -> KeyValuePair& = default;

        // Destructor
        ~KeyValuePair() = default;

        // Methods
        [[nodiscard]] auto get_key_id() const -> SchemaTreeNode::id_t { return m_key_id; }

        [[nodiscard]] auto get_value() const -> std::optional<Value> const& { return m_value; }

    private:
        SchemaTreeNode::id_t m_key_id;
        std::optional<Value> m_value;
    };

    // Factory functions
    /**
     * Creates a key-value pair log event from valid given inputs.
     * @param schema_tree
     * @param key_value_pair
     * @param utc_offset
     * @return A result containing the key-value pair log event or an error code indicating the
     * failure:
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<SchemaTree> schema_tree,
            std::vector<KeyValuePair> m_key_value_pairs,
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

    [[nodiscard]] auto get_key_value_pairs() const -> std::vector<KeyValuePair> const& {
        return m_key_value_pairs;
    }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

private:
    // Constructor
    KeyValuePairLogEvent(
            std::shared_ptr<SchemaTree> schema_tree,
            std::vector<KeyValuePair> key_value_pairs,
            UtcOffset utc_offset
    )
            : m_schema_tree{std::move(schema_tree)},
              m_key_value_pairs{std::move(key_value_pairs)},
              m_utc_offset{utc_offset} {}

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::vector<KeyValuePair> m_key_value_pairs;
    UtcOffset m_utc_offset{0};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_KEYVALUEPAIRLOGEVENT_HPP

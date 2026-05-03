#include "UnstructuredIrDeserializerImpl.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "../Value.hpp"
#include "decoding_methods.hpp"
#include "IrUnitType.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
namespace {
constexpr std::string_view cTimestampKey{"timestamp"};
constexpr std::string_view cMessageKey{"message"};

/**
 * Creates the schema node insertions emitted for old-IR log events.
 * NOTE: The insertion emitted first is `message` because deserialization pops from the back.
 * @return A vector used as a stack of pending insertions.
 */
[[nodiscard]] auto create_schema_insertions()
        -> std::vector<std::pair<bool, SchemaTree::NodeLocator>> {
    return {
            {true,
             SchemaTree::NodeLocator{
                     SchemaTree::cRootId,
                     cTimestampKey,
                     SchemaTree::Node::Type::Int
             }},
            {false,
             SchemaTree::NodeLocator{SchemaTree::cRootId, cMessageKey, SchemaTree::Node::Type::Str}}
    };
}
}  // namespace

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto UnstructuredIrDeserializerImpl<encoded_variable_t>::create(
        EncodingType encoding_type,
        nlohmann::json const& metadata
) -> ystdlib::error_handling::Result<std::unique_ptr<UnstructuredIrDeserializerImpl>> {
    auto initial_insertions{create_schema_insertions()};

    if constexpr (std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>) {
        if (EncodingType::EightByte != encoding_type) {
            return std::errc::protocol_error;
        }
        return std::make_unique<UnstructuredIrDeserializerImpl>(
                UnstructuredIrDeserializerImpl{std::move(initial_insertions)}
        );
    } else {  // std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
        if (EncodingType::FourByte != encoding_type) {
            return std::errc::protocol_error;
        }
        auto const ref_timestamp_iter{metadata.find(cProtocol::Metadata::ReferenceTimestampKey)};
        if (metadata.end() == ref_timestamp_iter || false == ref_timestamp_iter->is_string()) {
            return std::errc::protocol_error;
        }
        auto const& ref_timestamp_str{ref_timestamp_iter->get_ref<std::string const&>()};
        ir::epoch_time_ms_t ref_timestamp{};
        if (false == string_utils::convert_string_to_int(ref_timestamp_str, ref_timestamp)) {
            return std::errc::protocol_error;
        }
        return std::unique_ptr<UnstructuredIrDeserializerImpl>{
                new UnstructuredIrDeserializerImpl{std::move(initial_insertions), ref_timestamp}
        };
    }
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto UnstructuredIrDeserializerImpl<encoded_variable_t>::get_next_ir_unit_type(
        ReaderInterface& reader
) -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> {
    if (false == m_pending_schema_insertions.empty()) {
        return {IrUnitType::SchemaTreeNodeInsertion, encoded_tag_t{0}};
    }

    auto const tag{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_tag(reader))};

    if (cProtocol::Eof == tag) {
        return {IrUnitType::EndOfStream, tag};
    }
    if (cProtocol::Payload::UtcOffsetChange == tag) {
        return {IrUnitType::UtcOffsetChange, tag};
    }

    return {IrUnitType::LogEvent, tag};
}

template <>
auto UnstructuredIrDeserializerImpl<ir::four_byte_encoded_variable_t>::
        deserialize_ir_unit_kv_pair_log_event(
                ReaderInterface& reader,
                encoded_tag_t tag,
                std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
                std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
                UtcOffset utc_offset
        ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> {
    auto encoded_text_ast{YSTDLIB_ERROR_HANDLING_TRYX(
            deserialize_encoded_text_ast<ir::four_byte_encoded_variable_t>(reader, tag)
    )};
    auto const timestamp_tag{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_tag(reader))};
    ir::epoch_time_ms_t timestamp_delta{};
    if (auto const err{deserialize_timestamp<ir::four_byte_encoded_variable_t>(
                reader,
                timestamp_tag,
                timestamp_delta
        )};
        IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }
    m_previous_timestamp += timestamp_delta;

    auto const [message_node_id, timestamp_node_id]{YSTDLIB_ERROR_HANDLING_TRYX(
            this->resolve_required_node_ids(auto_gen_keys_schema_tree, user_gen_keys_schema_tree)
    )};

    KeyValuePairLogEvent::NodeIdValuePairs auto_gen_pairs{
            {timestamp_node_id, Value{static_cast<value_int_t>(m_previous_timestamp)}}
    };
    KeyValuePairLogEvent::NodeIdValuePairs user_gen_pairs{
            {message_node_id, Value{std::move(encoded_text_ast)}}
    };

    return KeyValuePairLogEvent::create(
            auto_gen_keys_schema_tree,
            user_gen_keys_schema_tree,
            std::move(auto_gen_pairs),
            std::move(user_gen_pairs),
            utc_offset
    );
}

template <>
auto UnstructuredIrDeserializerImpl<ir::eight_byte_encoded_variable_t>::
        deserialize_ir_unit_kv_pair_log_event(
                ReaderInterface& reader,
                encoded_tag_t tag,
                std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
                std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree,
                UtcOffset utc_offset
        ) -> ystdlib::error_handling::Result<KeyValuePairLogEvent> {
    auto encoded_text_ast{YSTDLIB_ERROR_HANDLING_TRYX(
            deserialize_encoded_text_ast<ir::eight_byte_encoded_variable_t>(reader, tag)
    )};
    auto const timestamp_tag{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_tag(reader))};
    ir::epoch_time_ms_t absolute_timestamp{};
    if (auto const err{deserialize_timestamp<ir::eight_byte_encoded_variable_t>(
                reader,
                timestamp_tag,
                absolute_timestamp
        )};
        IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    auto const [message_node_id, timestamp_node_id]{YSTDLIB_ERROR_HANDLING_TRYX(
            this->resolve_required_node_ids(auto_gen_keys_schema_tree, user_gen_keys_schema_tree)
    )};

    KeyValuePairLogEvent::NodeIdValuePairs auto_gen_pairs;
    auto_gen_pairs.emplace(timestamp_node_id, Value{static_cast<value_int_t>(absolute_timestamp)});
    KeyValuePairLogEvent::NodeIdValuePairs user_gen_pairs;
    user_gen_pairs.emplace(message_node_id, Value{std::move(encoded_text_ast)});

    return KeyValuePairLogEvent::create(
            auto_gen_keys_schema_tree,
            user_gen_keys_schema_tree,
            std::move(auto_gen_pairs),
            std::move(user_gen_pairs),
            utc_offset
    );
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto
UnstructuredIrDeserializerImpl<encoded_variable_t>::deserialize_ir_unit_schema_tree_node_insertion(
        [[maybe_unused]] ReaderInterface& reader,
        [[maybe_unused]] encoded_tag_t tag,
        std::string& key_name
) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> {
    auto const [is_auto_gen, node_locator]{m_pending_schema_insertions.back()};
    m_pending_schema_insertions.pop_back();

    key_name.clear();
    return {is_auto_gen, node_locator};
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
template <typename T, std::enable_if_t<std::is_same_v<T, ir::eight_byte_encoded_variable_t>, int>>
UnstructuredIrDeserializerImpl<encoded_variable_t>::UnstructuredIrDeserializerImpl(
        std::vector<std::pair<bool, SchemaTree::NodeLocator>> initial_insertions
)
        : m_pending_schema_insertions{std::move(initial_insertions)} {}

template <ir::EncodedVariableTypeReq encoded_variable_t>
template <typename T, std::enable_if_t<std::is_same_v<T, ir::four_byte_encoded_variable_t>, int>>
UnstructuredIrDeserializerImpl<encoded_variable_t>::UnstructuredIrDeserializerImpl(
        std::vector<std::pair<bool, SchemaTree::NodeLocator>> initial_insertions,
        ir::epoch_time_ms_t reference_timestamp
)
        : m_pending_schema_insertions{std::move(initial_insertions)},
          m_previous_timestamp{reference_timestamp} {}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto UnstructuredIrDeserializerImpl<encoded_variable_t>::resolve_required_node_ids(
        std::shared_ptr<SchemaTree> const& auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree
) -> ystdlib::error_handling::Result<std::pair<SchemaTree::Node::id_t, SchemaTree::Node::id_t>> {
    if (m_optional_message_node_id.has_value() && m_optional_timestamp_node_id.has_value()) {
        return {m_optional_message_node_id.value(), m_optional_timestamp_node_id.value()};
    }

    auto const message_node_id{user_gen_keys_schema_tree->try_get_node_id(
            SchemaTree::NodeLocator{SchemaTree::cRootId, cMessageKey, SchemaTree::Node::Type::Str}
    )};
    auto const timestamp_node_id{auto_gen_keys_schema_tree->try_get_node_id(
            SchemaTree::NodeLocator{SchemaTree::cRootId, cTimestampKey, SchemaTree::Node::Type::Int}
    )};
    if (false == message_node_id.has_value() || false == timestamp_node_id.has_value()) {
        return std::errc::protocol_error;
    }

    m_optional_message_node_id.emplace(message_node_id.value());
    m_optional_timestamp_node_id.emplace(timestamp_node_id.value());
    return {message_node_id.value(), timestamp_node_id.value()};
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template class UnstructuredIrDeserializerImpl<ir::four_byte_encoded_variable_t>;
template class UnstructuredIrDeserializerImpl<ir::eight_byte_encoded_variable_t>;
}  // namespace clp::ffi::ir_stream

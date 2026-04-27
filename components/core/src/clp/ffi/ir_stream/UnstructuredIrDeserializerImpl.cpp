#include "UnstructuredIrDeserializerImpl.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../EncodedTextAst.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "../StringBlob.hpp"
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
[[nodiscard]] auto create_schema_insertions() -> std::vector<SchemaTree::NodeLocator> {
    return {SchemaTree::NodeLocator{
                    SchemaTree::cRootId,
                    cTimestampKey,
                    SchemaTree::Node::Type::Int
            },
            SchemaTree::NodeLocator{SchemaTree::cRootId, cMessageKey, SchemaTree::Node::Type::Str}};
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
/**
 * Builds an encoded text AST from the components of an unstructured IR log event.
 * @tparam encoded_variable_t
 * @param logtype
 * @param dict_vars
 * @param encoded_vars
 * @return A result containing the encoded text AST on success, or an error code indicating the
 * failure:
 * - Forwards `EncodedTextAst::create`'s return values on failure.
 */
[[nodiscard]] auto build_encoded_text_ast(
        std::string const& logtype,
        std::vector<std::string> const& dict_vars,
        std::vector<encoded_variable_t> encoded_vars
) -> ystdlib::error_handling::Result<EncodedTextAst<encoded_variable_t>> {
    StringBlob string_blob;
    for (auto const& dict_var : dict_vars) {
        string_blob.append(dict_var);
    }
    string_blob.append(logtype);

    return EncodedTextAst<encoded_variable_t>::create(
            std::move(encoded_vars),
            std::move(string_blob)
    );
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
        return std::unique_ptr<UnstructuredIrDeserializerImpl>{
                new UnstructuredIrDeserializerImpl{std::move(initial_insertions)}
        };
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
UnstructuredIrDeserializerImpl<encoded_variable_t>::UnstructuredIrDeserializerImpl(
        std::vector<SchemaTree::NodeLocator> initial_insertions
)
        : m_pending_schema_insertions{std::move(initial_insertions)} {}

template <ir::EncodedVariableTypeReq encoded_variable_t>
UnstructuredIrDeserializerImpl<encoded_variable_t>::UnstructuredIrDeserializerImpl(
        std::vector<SchemaTree::NodeLocator> initial_insertions,
        ir::epoch_time_ms_t reference_timestamp
)
        : m_pending_schema_insertions{std::move(initial_insertions)},
          m_previous_timestamp{reference_timestamp} {}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto UnstructuredIrDeserializerImpl<encoded_variable_t>::resolve_required_node_ids(
        std::shared_ptr<SchemaTree> const& user_gen_keys_schema_tree
) -> ystdlib::error_handling::Result<std::pair<SchemaTree::Node::id_t, SchemaTree::Node::id_t>> {
    if (m_optional_message_node_id.has_value() && m_optional_timestamp_node_id.has_value()) {
        return std::pair{m_optional_message_node_id.value(), m_optional_timestamp_node_id.value()};
    }

    auto const message_node_id{user_gen_keys_schema_tree->try_get_node_id(
            SchemaTree::NodeLocator{SchemaTree::cRootId, cMessageKey, SchemaTree::Node::Type::Str}
    )};
    auto const timestamp_node_id{user_gen_keys_schema_tree->try_get_node_id(
            SchemaTree::NodeLocator{SchemaTree::cRootId, cTimestampKey, SchemaTree::Node::Type::Int}
    )};
    if (false == message_node_id.has_value() || false == timestamp_node_id.has_value()) {
        return std::errc::protocol_error;
    }

    m_optional_message_node_id.emplace(message_node_id.value());
    m_optional_timestamp_node_id.emplace(timestamp_node_id.value());
    return std::pair{message_node_id.value(), timestamp_node_id.value()};
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto UnstructuredIrDeserializerImpl<encoded_variable_t>::get_next_ir_unit_type(
        ReaderInterface& reader
) -> ystdlib::error_handling::Result<std::pair<IrUnitType, encoded_tag_t>> {
    if (false == m_pending_schema_insertions.empty()) {
        return std::pair{IrUnitType::SchemaTreeNodeInsertion, encoded_tag_t{0}};
    }

    auto const tag{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_tag(reader))};

    if (cProtocol::Eof == tag) {
        return std::pair{IrUnitType::EndOfStream, tag};
    }
    if (cProtocol::Payload::UtcOffsetChange == tag) {
        return std::pair{IrUnitType::UtcOffsetChange, tag};
    }

    return {IrUnitType::LogEvent, tag};
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
auto
UnstructuredIrDeserializerImpl<encoded_variable_t>::deserialize_ir_unit_schema_tree_node_insertion(
        [[maybe_unused]] ReaderInterface& reader,
        [[maybe_unused]] encoded_tag_t tag,
        std::string& key_name
) -> ystdlib::error_handling::Result<std::pair<bool, SchemaTree::NodeLocator>> {
    auto const node_locator{m_pending_schema_insertions.back()};
    m_pending_schema_insertions.pop_back();

    key_name.clear();
    return std::pair{false, node_locator};
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
    std::string logtype;
    std::vector<ir::four_byte_encoded_variable_t> encoded_vars;
    std::vector<std::string> dict_vars;
    ir::epoch_time_ms_t timestamp_delta{};

    if (auto const err{deserialize_log_event<ir::four_byte_encoded_variable_t>(
                reader,
                tag,
                logtype,
                encoded_vars,
                dict_vars,
                timestamp_delta
        )};
        IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    m_previous_timestamp += timestamp_delta;

    auto encoded_text_ast_result{build_encoded_text_ast<ir::four_byte_encoded_variable_t>(
            logtype,
            dict_vars,
            std::move(encoded_vars)
    )};
    if (encoded_text_ast_result.has_error()) {
        return encoded_text_ast_result.error();
    }

    auto const [message_node_id, timestamp_node_id]{
            YSTDLIB_ERROR_HANDLING_TRYX(this->resolve_required_node_ids(user_gen_keys_schema_tree))
    };

    KeyValuePairLogEvent::NodeIdValuePairs user_gen_pairs;
    user_gen_pairs.emplace(message_node_id, Value{std::move(encoded_text_ast_result.value())});
    user_gen_pairs.emplace(
            timestamp_node_id,
            Value{static_cast<value_int_t>(m_previous_timestamp)}
    );

    return KeyValuePairLogEvent::create(
            auto_gen_keys_schema_tree,
            user_gen_keys_schema_tree,
            {},
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
    std::string logtype;
    std::vector<ir::eight_byte_encoded_variable_t> encoded_vars;
    std::vector<std::string> dict_vars;
    ir::epoch_time_ms_t absolute_timestamp{};

    if (auto const err{deserialize_log_event<ir::eight_byte_encoded_variable_t>(
                reader,
                tag,
                logtype,
                encoded_vars,
                dict_vars,
                absolute_timestamp
        )};
        IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    auto encoded_text_ast_result{build_encoded_text_ast<ir::eight_byte_encoded_variable_t>(
            logtype,
            dict_vars,
            std::move(encoded_vars)
    )};
    if (encoded_text_ast_result.has_error()) {
        return encoded_text_ast_result.error();
    }

    auto const [message_node_id, timestamp_node_id]{
            YSTDLIB_ERROR_HANDLING_TRYX(this->resolve_required_node_ids(user_gen_keys_schema_tree))
    };

    KeyValuePairLogEvent::NodeIdValuePairs user_gen_pairs;
    user_gen_pairs.emplace(message_node_id, Value{std::move(encoded_text_ast_result.value())});
    user_gen_pairs.emplace(timestamp_node_id, Value{static_cast<value_int_t>(absolute_timestamp)});

    return KeyValuePairLogEvent::create(
            auto_gen_keys_schema_tree,
            user_gen_keys_schema_tree,
            {},
            std::move(user_gen_pairs),
            utc_offset
    );
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template class UnstructuredIrDeserializerImpl<ir::four_byte_encoded_variable_t>;
template class UnstructuredIrDeserializerImpl<ir::eight_byte_encoded_variable_t>;
}  // namespace clp::ffi::ir_stream

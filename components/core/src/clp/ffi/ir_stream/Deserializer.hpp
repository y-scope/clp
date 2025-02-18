#ifndef CLP_FFI_IR_STREAM_DESERIALIZER_HPP
#define CLP_FFI_IR_STREAM_DESERIALIZER_HPP

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"
#include "ir_unit_deserialization_methods.hpp"
#include "IrUnitHandlerInterface.hpp"
#include "IrUnitType.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
/**
 * A deserializer for reading IR units from a CLP kv-pair IR stream. An IR unit handler should be
 * provided to perform user-defined operations on each deserialized IR unit.
 *
 * NOTE: This class is designed only to provide deserialization functionalities. Callers are
 * responsible for maintaining a `ReaderInterface` to input IR bytes from an I/O stream.
 *
 * @tparam IrUnitHandler
 */
template <IrUnitHandlerInterface IrUnitHandler>
requires(std::move_constructible<IrUnitHandler>)
class Deserializer {
public:
    // Factory function
    /**
     * Creates a deserializer by reading the stream's preamble from the given reader.
     * @param reader
     * @param ir_unit_handler
     * @return A result containing the deserializer or an error code indicating the failure:
     * - std::errc::result_out_of_range if the IR stream is truncated
     * - std::errc::protocol_error if the IR stream is corrupted
     * - std::errc::protocol_not_supported if either:
     *   - the IR stream contains an unsupported metadata format;
     *   - the IR stream's version is unsupported;
     *   - or the IR stream's user-defined metadata is not a JSON object.
     */
    [[nodiscard]] static auto create(ReaderInterface& reader, IrUnitHandler ir_unit_handler)
            -> OUTCOME_V2_NAMESPACE::std_result<Deserializer>;

    // Delete copy constructor and assignment
    Deserializer(Deserializer const&) = delete;
    auto operator=(Deserializer const&) -> Deserializer& = delete;

    // Define default move constructor and assignment
    Deserializer(Deserializer&&) = default;
    auto operator=(Deserializer&&) -> Deserializer& = default;

    // Destructor
    ~Deserializer() = default;

    // Methods
    /**
     * Deserializes the stream from the given reader up to and including the next log event IR unit.
     * @param reader
     * @return Forwards `deserialize_tag`s return values if no tag bytes can be read to determine
     * the next IR unit type.
     * @return std::errc::protocol_not_supported if the IR unit type is not supported.
     * @return std::errc::operation_not_permitted if the deserializer already reached the end of
     * stream by deserializing an end-of-stream IR unit in the previous calls.
     * @return IRUnitType::LogEvent if a log event IR unit is deserialized, or an error code
     * indicating the failure:
     * - Forwards `deserialize_ir_unit_kv_pair_log_event`'s return values if it failed to
     *   deserialize and construct the log event.
     * - Forwards `handle_log_event`'s return values from the user-defined IR unit handler on
     *   unit handling failure.
     * @return IRUnitType::SchemaTreeNodeInsertion if a schema tree node insertion IR unit is
     * deserialized, or an error code indicating the failure:
     * - Forwards `deserialize_ir_unit_schema_tree_node_insertion`'s return values if it failed to
     *   deserialize and construct the schema tree node locator.
     * - Forwards `handle_schema_tree_node_insertion`'s return values from the user-defined IR unit
     *   handler on unit handling failure.
     * - std::errc::protocol_error if the deserialized schema tree node already exists in the schema
     *   tree.
     * @return IRUnitType::UtcOffsetChange if a UTC offset change IR unit is deserialized, or an
     * error code indicating the failure:
     * - Forwards `deserialize_ir_unit_utc_offset_change`'s return values if it failed to
     *   deserialize the UTC offset.
     * - Forwards `handle_utc_offset_change`'s return values from the user-defined IR unit handler
     *   on unit handling failure.
     * @return IRUnitType::EndOfStream if an end-of-stream IR unit is deserialized, or an error code
     * indicating the failure:
     * - Forwards `handle_end_of_stream`'s return values from the user-defined IR unit handler on
     *   unit handling failure.
     */
    [[nodiscard]] auto deserialize_next_ir_unit(ReaderInterface& reader)
            -> OUTCOME_V2_NAMESPACE::std_result<IrUnitType>;

    /**
     * @return Whether the stream has completed. A stream is considered completed if an
     * end-of-stream IR unit has already been deserialized.
     */
    [[nodiscard]] auto is_stream_completed() const -> bool { return m_is_complete; }

    [[nodiscard]] auto get_ir_unit_handler() const -> IrUnitHandler const& {
        return m_ir_unit_handler;
    }

    [[nodiscard]] auto get_ir_unit_handler() -> IrUnitHandler& { return m_ir_unit_handler; }

    /**
     * @return The metadata associated with the deserialized stream.
     */
    [[nodiscard]] auto get_metadata() const -> nlohmann::json const& { return m_metadata; }

private:
    // Constructor
    Deserializer(IrUnitHandler ir_unit_handler, nlohmann::json metadata)
            : m_ir_unit_handler{std::move(ir_unit_handler)},
              m_metadata(std::move(metadata)) {}

    // Variables
    std::shared_ptr<SchemaTree> m_auto_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    std::shared_ptr<SchemaTree> m_user_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    nlohmann::json m_metadata;
    UtcOffset m_utc_offset{0};
    IrUnitHandler m_ir_unit_handler;
    bool m_is_complete{false};
};

template <IrUnitHandlerInterface IrUnitHandler>
requires(std::move_constructible<IrUnitHandler>)
auto Deserializer<IrUnitHandler>::create(ReaderInterface& reader, IrUnitHandler ir_unit_handler)
        -> OUTCOME_V2_NAMESPACE::std_result<Deserializer> {
    bool is_four_byte_encoded{};
    if (auto const err{get_encoding_type(reader, is_four_byte_encoded)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    std::vector<int8_t> metadata;
    encoded_tag_t metadata_type{};
    if (auto const err{deserialize_preamble(reader, metadata_type, metadata)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        return ir_error_code_to_errc(err);
    }

    if (cProtocol::Metadata::EncodingJson != metadata_type) {
        return std::errc::protocol_not_supported;
    }

    auto metadata_json = nlohmann::json::parse(metadata, nullptr, false);
    if (metadata_json.is_discarded()) {
        return std::errc::protocol_error;
    }
    auto const version_iter{metadata_json.find(cProtocol::Metadata::VersionKey)};
    if (metadata_json.end() == version_iter || false == version_iter->is_string()) {
        return std::errc::protocol_error;
    }
    auto const version = version_iter->get_ref<nlohmann::json::string_t&>();
    if (ffi::ir_stream::IRProtocolErrorCode::Supported
        != ffi::ir_stream::validate_protocol_version(version))
    {
        return std::errc::protocol_not_supported;
    }

    if (metadata_json.contains(cProtocol::Metadata::UserDefinedMetadataKey)
        && false == metadata_json.at(cProtocol::Metadata::UserDefinedMetadataKey).is_object())
    {
        return std::errc::protocol_not_supported;
    }

    return Deserializer{std::move(ir_unit_handler), std::move(metadata_json)};
}

template <IrUnitHandlerInterface IrUnitHandler>
requires(std::move_constructible<IrUnitHandler>)
auto Deserializer<IrUnitHandler>::deserialize_next_ir_unit(ReaderInterface& reader)
        -> OUTCOME_V2_NAMESPACE::std_result<IrUnitType> {
    if (is_stream_completed()) {
        return std::errc::operation_not_permitted;
    }

    encoded_tag_t tag{};
    if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
        return ir_error_code_to_errc(err);
    }

    auto const optional_ir_unit_type{get_ir_unit_type_from_tag(tag)};
    if (false == optional_ir_unit_type.has_value()) {
        return std::errc::protocol_not_supported;
    }

    auto const ir_unit_type{optional_ir_unit_type.value()};
    switch (ir_unit_type) {
        case IrUnitType::LogEvent: {
            auto result{deserialize_ir_unit_kv_pair_log_event(
                    reader,
                    tag,
                    m_auto_gen_keys_schema_tree,
                    m_user_gen_keys_schema_tree,
                    m_utc_offset
            )};
            if (result.has_error()) {
                return result.error();
            }

            if (auto const err{m_ir_unit_handler.handle_log_event(std::move(result.value()))};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }
            break;
        }

        case IrUnitType::SchemaTreeNodeInsertion: {
            std::string key_name;
            auto const result{deserialize_ir_unit_schema_tree_node_insertion(reader, tag, key_name)
            };
            if (result.has_error()) {
                return result.error();
            }

            auto const& [is_auto_generated, node_locator]{result.value()};
            auto& schema_tree_to_insert{
                    is_auto_generated ? m_auto_gen_keys_schema_tree : m_user_gen_keys_schema_tree
            };

            if (schema_tree_to_insert->has_node(node_locator)) {
                return std::errc::protocol_error;
            }

            if (auto const err{m_ir_unit_handler.handle_schema_tree_node_insertion(
                        is_auto_generated,
                        node_locator
                )};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }

            std::ignore = schema_tree_to_insert->insert_node(node_locator);
            break;
        }

        case IrUnitType::UtcOffsetChange: {
            auto const result{deserialize_ir_unit_utc_offset_change(reader)};
            if (result.has_error()) {
                return result.error();
            }

            auto const new_utc_offset{result.value()};
            if (auto const err{
                        m_ir_unit_handler.handle_utc_offset_change(m_utc_offset, new_utc_offset)
                };
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }

            m_utc_offset = new_utc_offset;
            break;
        }

        case IrUnitType::EndOfStream: {
            if (auto const err{m_ir_unit_handler.handle_end_of_stream()};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }
            m_is_complete = true;
            break;
        }

        default:
            return std::errc::protocol_not_supported;
    }

    return ir_unit_type;
}
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DESERIALIZER_HPP

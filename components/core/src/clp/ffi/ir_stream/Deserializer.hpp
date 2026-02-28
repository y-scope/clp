#ifndef CLP_FFI_IR_STREAM_DESERIALIZER_HPP
#define CLP_FFI_IR_STREAM_DESERIALIZER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../SchemaTree.hpp"
#include "ir_unit_deserialization_methods.hpp"
#include "IrUnitHandlerReq.hpp"
#include "IrUnitType.hpp"
#include "protocol_constants.hpp"
#include "search/AstEvaluationResult.hpp"
#include "search/QueryHandlerReq.hpp"
#include "utils.hpp"

// This include has a circular dependency with the `.inc` file.
// The following clang-tidy suppression should be removed once the circular dependency is resolved.
// NOLINTNEXTLINE(misc-header-include-cycle)
#include "decoding_methods.hpp"

namespace clp::ffi::ir_stream {
/**
 * A deserializer for reading IR units from a CLP kv-pair IR stream. An IR unit handler should be
 * provided to perform user-defined operations on each deserialized IR unit. Additionally, a query
 * handler can be provided to handle queries and column projections.
 *
 * NOTE: This class is designed only to provide deserialization functionalities. Callers are
 * responsible for maintaining a `ReaderInterface` to input IR bytes from an I/O stream.
 *
 * @tparam IrUnitHandlerType
 * @tparam QueryHandlerType
 */
template <
        IrUnitHandlerReq IrUnitHandlerType,
        search::QueryHandlerReq QueryHandlerType = search::EmptyQueryHandler
>
class Deserializer {
public:
    // Factory function
    /**
     * Creates a deserializer with an empty query handler (for use when the deserializer won't be
     * used to perform queries or column projections).
     * @param reader
     * @param ir_unit_handler
     * @return A result containing the deserializer on success, or an error code indicating the
     * failure:
     * - Forwards `create_generic`'s return values.
     */
    [[nodiscard]] static auto create(ReaderInterface& reader, IrUnitHandlerType ir_unit_handler)
            -> ystdlib::error_handling::Result<Deserializer>
    requires std::is_same_v<QueryHandlerType, search::EmptyQueryHandler>
    {
        return create_generic(reader, std::move(ir_unit_handler), {});
    }

    /**
     * Creates a deserializer with a query handler (for use when the deserializer will be used to
     * perform queries or column projections).
     * @param reader
     * @param ir_unit_handler
     * @param query_handler
     * @return A result containing the deserializer on success, or an error code indicating the
     * failure:
     * - Forwards `create_generic`'s return values.
     */
    [[nodiscard]] static auto
    create(ReaderInterface& reader,
           IrUnitHandlerType ir_unit_handler,
           QueryHandlerType query_handler) -> ystdlib::error_handling::Result<Deserializer>
    requires search::IsNonEmptyQueryHandler<QueryHandlerType>::value
    {
        return create_generic(reader, std::move(ir_unit_handler), std::move(query_handler));
    }

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
     * Deserializes the stream from the given reader up to and including the next log event IR unit,
     * and invokes the user-defined IR unit handler according to the deserialized IR unit type.
     *
     * NOTE: If the deserialized IR unit is `IrUnitType::LogEvent` and the query handler is not
     * `search::EmptyQueryHandler`, `handle_log_event` will only be invoked if the query handler
     * returns `search::AstEvaluationResult::True`.
     *
     * @param reader
     * @return Forwards `deserialize_tag`s return values if no tag bytes can be read to determine
     * the next IR unit type.
     * @return std::errc::protocol_not_supported if the IR unit type is not supported.
     * @return std::errc::operation_not_permitted if the deserializer already reached the end of
     * stream by deserializing an end-of-stream IR unit in the previous calls.
     * @return IrUnitType::LogEvent if a log event IR unit is deserialized, or an error code
     * indicating the failure:
     * - Forwards `deserialize_ir_unit_kv_pair_log_event`'s return values if it failed to
     *   deserialize and construct the log event.
     * - Forwards `handle_log_event`'s return values from the user-defined IR unit handler on
     *   unit handling failure.
     * - Forwards `search::QueryHandler::evaluate_kv_pair_log_event`'s return values on failure, if
     *   `QueryHandlerType` is not `search::EmptyQueryHandler`.
     * @return IrUnitType::SchemaTreeNodeInsertion if a schema tree node insertion IR unit is
     * deserialized, or an error code indicating the failure:
     * - Forwards `deserialize_ir_unit_schema_tree_node_insertion`'s return values if it failed to
     *   deserialize and construct the schema tree node locator.
     * - Forwards `handle_schema_tree_node_insertion`'s return values from the user-defined IR unit
     *   handler on unit handling failure.
     * - Forwards `search::QueryHandler::update_partially_resolved_columns`'s return values on
     *   failure, if `QueryHandlerType` is not `search::EmptyQueryHandler`.
     * - std::errc::protocol_error if the deserialized schema tree node already exists in the
     *   schema tree.
     * @return IrUnitType::UtcOffsetChange if a UTC offset change IR unit is deserialized, or an
     * error code indicating the failure:
     * - Forwards `deserialize_ir_unit_utc_offset_change`'s return values if it failed to
     *   deserialize the UTC offset.
     * - Forwards `handle_utc_offset_change`'s return values from the user-defined IR unit handler
     *   on unit handling failure.
     * @return IrUnitType::EndOfStream if an end-of-stream IR unit is deserialized, or an error code
     * indicating the failure:
     * - Forwards `handle_end_of_stream`'s return values from the user-defined IR unit handler on
     *   unit handling failure.
     */
    [[nodiscard]] auto deserialize_next_ir_unit(ReaderInterface& reader)
            -> ystdlib::error_handling::Result<IrUnitType>;

    /**
     * @return Whether the stream has completed. A stream is considered completed if an
     * end-of-stream IR unit has already been deserialized.
     */
    [[nodiscard]] auto is_stream_completed() const -> bool { return m_is_complete; }

    [[nodiscard]] auto get_ir_unit_handler() const -> IrUnitHandlerType const& {
        return m_ir_unit_handler;
    }

    [[nodiscard]] auto get_ir_unit_handler() -> IrUnitHandlerType& { return m_ir_unit_handler; }

    /**
     * @return The metadata associated with the deserialized stream.
     */
    [[nodiscard]] auto get_metadata() const -> nlohmann::json const& { return m_metadata; }

    /**
     * @return The number of log events (log event IR units) that have been deserialized from the
     * current stream.
     */
    [[nodiscard]] auto get_num_log_events_deserialized() const -> size_t {
        return m_next_log_event_idx;
    }

private:
    // Factory function
    /**
     * Creates a deserializer by reading the stream's preamble from the given reader.
     * @param reader
     * @param ir_unit_handler
     * @param query_handler
     * @return A result containing the deserializer or an error code indicating the failure:
     * - std::errc::result_out_of_range if the IR stream is truncated
     * - std::errc::protocol_error if the IR stream is corrupted
     * - std::errc::protocol_not_supported if either:
     *   - the IR stream contains an unsupported metadata format;
     *   - the IR stream's version is unsupported;
     *   - or the IR stream's user-defined metadata is not a JSON object.
     */
    [[nodiscard]] static auto create_generic(
            ReaderInterface& reader,
            IrUnitHandlerType ir_unit_handler,
            QueryHandlerType query_handler
    ) -> ystdlib::error_handling::Result<Deserializer>;

    // Constructor
    Deserializer(
            IrUnitHandlerType ir_unit_handler,
            nlohmann::json metadata,
            QueryHandlerType query_handler
    )
            : m_metadata(std::move(metadata)),
              m_ir_unit_handler{std::move(ir_unit_handler)},
              m_query_handler{std::move(query_handler)} {}

    // Variables
    std::shared_ptr<SchemaTree> m_auto_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    std::shared_ptr<SchemaTree> m_user_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    nlohmann::json m_metadata;
    UtcOffset m_utc_offset{0};
    IrUnitHandlerType m_ir_unit_handler;
    bool m_is_complete{false};
    [[no_unique_address]] QueryHandlerType m_query_handler;
    size_t m_next_log_event_idx{0};
};

/**
 * Wrapper for `Deserializer`'s factory function to enable automatic type deduction.
 * @param reader
 * @param ir_unit_handler
 * @return Forwards `Deserializer::create`'s return values.
 */
template <IrUnitHandlerReq IrUnitHandler>
[[nodiscard]] auto make_deserializer(ReaderInterface& reader, IrUnitHandler ir_unit_handler)
        -> ystdlib::error_handling::Result<Deserializer<IrUnitHandler>>;

/**
 * Wrapper for `Deserializer`'s factory function to enable automatic type deduction.
 * @param reader
 * @param ir_unit_handler
 * @param query_handler
 * @return Forwards `Deserializer::create`'s return values.
 */
template <IrUnitHandlerReq IrUnitHandler, search::QueryHandlerReq QueryHandlerType>
[[nodiscard]] auto make_deserializer(
        ReaderInterface& reader,
        IrUnitHandler ir_unit_handler,
        QueryHandlerType query_handler
) -> ystdlib::error_handling::Result<Deserializer<IrUnitHandler, QueryHandlerType>>;

template <IrUnitHandlerReq IrUnitHandlerType, search::QueryHandlerReq QueryHandlerType>
auto Deserializer<IrUnitHandlerType, QueryHandlerType>::create_generic(
        ReaderInterface& reader,
        IrUnitHandlerType ir_unit_handler,
        QueryHandlerType query_handler
) -> ystdlib::error_handling::Result<Deserializer> {
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

    return Deserializer{
            std::move(ir_unit_handler),
            std::move(metadata_json),
            std::move(query_handler)
    };
}

template <IrUnitHandlerReq IrUnitHandler, search::QueryHandlerReq QueryHandlerType>
auto Deserializer<IrUnitHandler, QueryHandlerType>::deserialize_next_ir_unit(
        ReaderInterface& reader
) -> ystdlib::error_handling::Result<IrUnitType> {
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
            auto log_event{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_ir_unit_kv_pair_log_event(
                    reader,
                    tag,
                    m_auto_gen_keys_schema_tree,
                    m_user_gen_keys_schema_tree,
                    m_utc_offset
            ))};

            auto const log_event_idx{m_next_log_event_idx};
            m_next_log_event_idx += 1;

            if constexpr (search::IsNonEmptyQueryHandler<QueryHandlerType>::value) {
                if (search::AstEvaluationResult::True
                    != YSTDLIB_ERROR_HANDLING_TRYX(
                            m_query_handler.evaluate_kv_pair_log_event(log_event)
                    ))
                {
                    break;
                }
            }

            if (auto const err{
                        m_ir_unit_handler.handle_log_event(std::move(log_event), log_event_idx)
                };
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }
            break;
        }

        case IrUnitType::SchemaTreeNodeInsertion: {
            std::string key_name;
            auto const [is_auto_generated, node_locator]{YSTDLIB_ERROR_HANDLING_TRYX(
                    deserialize_ir_unit_schema_tree_node_insertion(reader, tag, key_name)
            )};
            auto& schema_tree_to_insert{
                    is_auto_generated ? m_auto_gen_keys_schema_tree : m_user_gen_keys_schema_tree
            };

            if (schema_tree_to_insert->has_node(node_locator)) {
                return std::errc::protocol_error;
            }

            auto const node_id{schema_tree_to_insert->insert_node(node_locator)};

            if constexpr (search::IsNonEmptyQueryHandler<QueryHandlerType>::value) {
                YSTDLIB_ERROR_HANDLING_TRYV(m_query_handler.update_partially_resolved_columns(
                        is_auto_generated,
                        node_locator,
                        node_id
                ));
            }

            if (auto const err{m_ir_unit_handler.handle_schema_tree_node_insertion(
                        is_auto_generated,
                        node_locator,
                        schema_tree_to_insert
                )};
                IRErrorCode::IRErrorCode_Success != err)
            {
                return ir_error_code_to_errc(err);
            }
            break;
        }

        case IrUnitType::UtcOffsetChange: {
            auto const new_utc_offset{
                    YSTDLIB_ERROR_HANDLING_TRYX(deserialize_ir_unit_utc_offset_change(reader))
            };
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

template <IrUnitHandlerReq IrUnitHandlerType>
[[nodiscard]] auto make_deserializer(ReaderInterface& reader, IrUnitHandlerType ir_unit_handler)
        -> ystdlib::error_handling::Result<Deserializer<IrUnitHandlerType>> {
    return Deserializer<IrUnitHandlerType>::create(reader, std::move(ir_unit_handler));
}

template <IrUnitHandlerReq IrUnitHandlerType, search::QueryHandlerReq QueryHandlerType>
[[nodiscard]] auto make_deserializer(
        ReaderInterface& reader,
        IrUnitHandlerType ir_unit_handler,
        QueryHandlerType query_handler
) -> ystdlib::error_handling::Result<Deserializer<IrUnitHandlerType, QueryHandlerType>> {
    return Deserializer<IrUnitHandlerType, QueryHandlerType>::create(
            reader,
            std::move(ir_unit_handler),
            std::move(query_handler)
    );
}
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_DESERIALIZER_HPP

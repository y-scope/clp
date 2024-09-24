#include "Deserializer.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../ReaderInterface.hpp"
#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "decoding_methods.hpp"
#include "ir_unit_deserialization_methods.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
namespace {
/**
 * Class to perform different actions depending on whether a transaction succeeds or fails. The
 * default state assumes the transaction fails.
 * @tparam SuccessHandler A cleanup lambda to call on success.
 * @tparam FailureHandler A cleanup lambda to call on failure.
 */
template <typename SuccessHandler, typename FailureHandler>
requires(std::is_invocable_v<SuccessHandler> && std::is_invocable_v<FailureHandler>)
class TransactionManager {
public:
    // Constructor
    TransactionManager(SuccessHandler success_handler, FailureHandler failure_handler)
            : m_success_handler{success_handler},
              m_failure_handler{failure_handler} {}

    // Delete copy/move constructor and assignment
    TransactionManager(TransactionManager const&) = delete;
    TransactionManager(TransactionManager&&) = delete;
    auto operator=(TransactionManager const&) -> TransactionManager& = delete;
    auto operator=(TransactionManager&&) -> TransactionManager& = delete;

    // Destructor
    ~TransactionManager() {
        if (m_success) {
            m_success_handler();
        } else {
            m_failure_handler();
        }
    }

    // Methods
    /**
     * Marks the transaction as successful.
     */
    auto mark_success() -> void { m_success = true; }

private:
    // Variables
    SuccessHandler m_success_handler;
    FailureHandler m_failure_handler;
    bool m_success{false};
};

/**
 * @param tag
 * @return Whether the tag represents a schema tree node.
 */
[[nodiscard]] auto is_schema_tree_node_tag(encoded_tag_t tag) -> bool;

auto is_schema_tree_node_tag(encoded_tag_t tag) -> bool {
    return (tag & cProtocol::Payload::SchemaTreeNodeMask) == cProtocol::Payload::SchemaTreeNodeMask;
}
}  // namespace

auto Deserializer::create(ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<Deserializer> {
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
    // TODO: Just before the KV-pair IR format is formally released, we should replace this
    // hard-coded version check with `ffi::ir_stream::validate_protocol_version`.
    if (std::string_view{static_cast<char const*>(cProtocol::Metadata::BetaVersionValue)}
        != version)
    {
        return std::errc::protocol_not_supported;
    }

    return Deserializer{};
}

auto Deserializer::deserialize_to_next_log_event(clp::ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<KeyValuePairLogEvent> {
    auto const utc_offset_snapshot{m_utc_offset};
    m_schema_tree->take_snapshot();
    TransactionManager revert_manager{
            []() -> void {},
            [&]() -> void {
                m_utc_offset = utc_offset_snapshot;
                m_schema_tree->revert();
            }
    };

    encoded_tag_t tag{};
    std::string schema_tree_node_key_name;
    while (true) {
        if (auto const err{deserialize_tag(reader, tag)}; IRErrorCode::IRErrorCode_Success != err) {
            return ir_error_code_to_errc(err);
        }

        if (cProtocol::Eof == tag) {
            return std::errc::no_message_available;
        }

        if (is_schema_tree_node_tag(tag)) {
            auto const result{deserialize_ir_unit_schema_tree_node_insertion(
                    reader,
                    tag,
                    schema_tree_node_key_name
            )};
            if (result.has_error()) {
                return result.error();
            }
            auto const& locator{result.value()};
            if (m_schema_tree->has_node(locator)) {
                return std::errc::protocol_error;
            }
            std::ignore = m_schema_tree->insert_node(locator);
            continue;
        }

        if (cProtocol::Payload::UtcOffsetChange == tag) {
            auto const result{deserialize_ir_unit_utc_offset_change(reader)};
            if (result.has_error()) {
                return result.error();
            }
            m_utc_offset = result.value();
            continue;
        }

        break;
    }

    auto result{deserialize_ir_unit_kv_pair_log_event(reader, tag, m_schema_tree, m_utc_offset)};
    if (false == result.has_error()) {
        revert_manager.mark_success();
    }
    return std::move(result);
}
}  // namespace clp::ffi::ir_stream

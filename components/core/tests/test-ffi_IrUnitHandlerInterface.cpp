#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/IrUnitHandlerInterface.hpp"
#include "../src/clp/ffi/KeyValuePairLogEvent.hpp"
#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/time_types.hpp"

namespace {
using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::KeyValuePairLogEvent;
using clp::ffi::SchemaTree;
using clp::UtcOffset;

constexpr UtcOffset cTestUtcOffset{100};
constexpr UtcOffset cTestUtcOffsetDelta{1000};
constexpr std::string_view cTestSchemaTreeNodeKeyName{"test_key"};

/**
 * Class that implements `clp::ffi::ir_stream::IrUnitHandlerInterface` for testing purposes.
 */
class TrivialIrUnitHandler {
public:
    // Implements `clp::ffi::ir_stream::IrUnitHandlerInterface` interface
    [[nodiscard]] auto handle_log_event(KeyValuePairLogEvent&& log_event) -> IRErrorCode {
        m_log_event.emplace(std::move(log_event));
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_utc_offset_change(UtcOffset utc_offset_old, UtcOffset utc_offset_new)
            -> IRErrorCode {
        m_utc_offset_delta = utc_offset_new - utc_offset_old;
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            SchemaTree::NodeLocator schema_tree_node_locator
    ) -> IRErrorCode {
        m_schema_tree_node_locator.emplace(schema_tree_node_locator);
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_end_of_stream() -> IRErrorCode {
        m_is_complete = true;
        return IRErrorCode::IRErrorCode_Success;
    }

    // Methods
    [[nodiscard]] auto get_utc_offset_delta() const -> UtcOffset { return m_utc_offset_delta; }

    [[nodiscard]] auto is_complete() const -> bool { return m_is_complete; }

    [[nodiscard]] auto get_schema_tree_node_locator() const
            -> std::optional<SchemaTree::NodeLocator> const& {
        return m_schema_tree_node_locator;
    }

    [[nodiscard]] auto get_log_event() const -> std::optional<KeyValuePairLogEvent> const& {
        return m_log_event;
    }

private:
    UtcOffset m_utc_offset_delta{0};
    bool m_is_complete{false};
    std::optional<SchemaTree::NodeLocator> m_schema_tree_node_locator;
    std::optional<KeyValuePairLogEvent> m_log_event;
};

/**
 * Class that inherits `TrivialIrUnitHandler` which also implements
 * `clp::ffi::ir_stream::IrUnitHandlerInterface`.
 */
class TriviallyInheritedIrUnitHandler : public TrivialIrUnitHandler {};

/**
 * Simulates the use of an IR unit handler. It calls every method required by
 * `clp::ffi::ir_stream::IrUnitHandlerInterface` and ensure they don't return errors.
 * @param handler
 */
auto test_ir_unit_handler_interface(clp::ffi::ir_stream::IrUnitHandlerInterface auto& handler)
        -> void;

auto test_ir_unit_handler_interface(clp::ffi::ir_stream::IrUnitHandlerInterface auto& handler)
        -> void {
    auto test_log_event_result{KeyValuePairLogEvent::create(
            std::make_shared<SchemaTree>(),
            std::make_shared<SchemaTree>(),
            {},
            {},
            cTestUtcOffset
    )};
    REQUIRE(
            (false == test_log_event_result.has_error()
             && IRErrorCode::IRErrorCode_Success
                        == handler.handle_log_event(std::move(test_log_event_result.value())))
    );
    REQUIRE(
            (IRErrorCode::IRErrorCode_Success
             == handler.handle_utc_offset_change(
                     cTestUtcOffset,
                     cTestUtcOffset + cTestUtcOffsetDelta
             ))
    );
    REQUIRE(
            (IRErrorCode::IRErrorCode_Success
             == handler.handle_schema_tree_node_insertion(
                     true,
                     {SchemaTree::cRootId, cTestSchemaTreeNodeKeyName, SchemaTree::Node::Type::Obj}
             ))
    );
    REQUIRE((IRErrorCode::IRErrorCode_Success == handler.handle_end_of_stream()));
}
}  // namespace

TEMPLATE_TEST_CASE(
        "test_ir_unit_handler_interface_basic",
        "[ffi][ir_stream]",
        TrivialIrUnitHandler,
        TriviallyInheritedIrUnitHandler
) {
    TestType handler;
    REQUIRE_FALSE(handler.is_complete());
    test_ir_unit_handler_interface(handler);

    REQUIRE((handler.get_utc_offset_delta() == cTestUtcOffsetDelta));
    auto const& optional_log_event{handler.get_log_event()};
    REQUIRE(
            (optional_log_event.has_value()
             && optional_log_event.value().get_utc_offset() == cTestUtcOffset
             && optional_log_event.value().get_user_gen_node_id_value_pairs().empty())
    );
    auto const& optional_schema_tree_locator{handler.get_schema_tree_node_locator()};
    REQUIRE(
            (optional_schema_tree_locator.has_value()
             && optional_schema_tree_locator.value().get_key_name() == cTestSchemaTreeNodeKeyName)
    );
    REQUIRE(handler.is_complete());
}

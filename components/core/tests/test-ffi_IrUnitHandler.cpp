#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/IrUnitHandler.hpp"
#include "../src/clp/ffi/KeyValuePairLogEvent.hpp"
#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/ffi/SchemaTreeNode.hpp"
#include "../src/clp/time_types.hpp"

using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::KeyValuePairLogEvent;
using clp::ffi::SchemaTree;
using clp::ffi::SchemaTreeNode;
using clp::UtcOffset;

namespace {
constexpr UtcOffset cTestUtcOffset{100};
constexpr UtcOffset cTestUtcOffsetDelta{1000};
constexpr std::string_view cTestSchemaTreeNodeKeyName{"test_key"};

class TestUnitHandler {
public:
    // Implements `clp::ffi::ir_stream::IrUnitHandler` interface
    [[nodiscard]] auto handle_log_event(KeyValuePairLogEvent log_event) -> IRErrorCode {
        m_log_event.emplace(std::move(log_event));
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto
    handle_utc_offset_change(UtcOffset utc_offset_old, UtcOffset utc_offset_new) -> IRErrorCode {
        m_utc_offset_delta = utc_offset_new - utc_offset_old;
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_schema_tree_node_insertion(
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

    [[nodiscard]] auto get_schema_tree_node_locator(
    ) const -> std::optional<SchemaTree::NodeLocator> const& {
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

template <clp::ffi::ir_stream::IrUnitHandler Handler>
auto test_ir_unit_handler(Handler& handler) -> void {
    auto test_log_event_result{
            KeyValuePairLogEvent::create(std::make_shared<SchemaTree>(), {}, cTestUtcOffset)
    };
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
                     {SchemaTree::cRootId, cTestSchemaTreeNodeKeyName, SchemaTreeNode::Type::Obj}
             ))
    );
    REQUIRE((IRErrorCode::IRErrorCode_Success == handler.handle_end_of_stream()));
}

TEST_CASE("test_ir_unit_handler_basic", "[ffi][ir_stream]") {
    TestUnitHandler handler;
    test_ir_unit_handler(handler);
}
}  // namespace

#ifndef FFI_IR_STREAM_ATTRIBUTES_HPP
#define FFI_IR_STREAM_ATTRIBUTES_HPP

#include <optional>
#include <string>
#include <variant>

namespace ffi::ir_stream {
class Attribute {
public:
    [[nodiscard]] auto is_string() const -> bool {
        return std::holds_alternative<std::string>(m_attribute);
    }
    [[nodiscard]] auto is_int() const -> bool {
        return std::holds_alternative<int64_t>(m_attribute);
    }
private:
    std::variant<std::string, int64_t> m_attribute;
};
}

#endif
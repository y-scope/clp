#ifndef CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP
#define CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "../../clp/ffi/ir_stream/protocol_constants.hpp"
#include "../../clp/ffi/ir_stream/Serializer.hpp"
#include "../../clp/ir/types.hpp"
#include "../FileWriter.hpp"

namespace clp_s::log_converter {
class LogSerializer {
public:
    // Disable copy constructor/assignment operator
    LogSerializer(LogSerializer const&) = delete;
    auto operator=(LogSerializer const&) -> LogSerializer& = delete;

    // Define default move constructor/assignment operator
    LogSerializer(LogSerializer&&) = default;
    auto operator=(LogSerializer&&) -> LogSerializer& = default;

    // Destructor
    ~LogSerializer() = default;

    static auto create(std::string_view output_dir, std::string_view original_file_path)
            -> std::optional<LogSerializer>;

    auto add_message(std::string_view timestamp, std::string_view message) -> bool;

    auto add_message(std::string_view message) -> bool;

    void close() {
        flush_buffer();
        m_writer.write_numeric_value(clp::ffi::ir_stream::cProtocol::Eof);
        m_writer.close();
    }

private:
    static constexpr std::string_view cOriginalFileMetadataKey{"original_file"};
    static constexpr std::string_view cTimestampKey{"timestamp"};
    static constexpr std::string_view cMessageKey{"message"};
    static constexpr size_t cMaxIrBufSize{64 * 1024};  // 64 KiB

    explicit LogSerializer(
            clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>&& serializer,
            clp_s::FileWriter&& writer
    )
            : m_serializer{std::move(serializer)},
              m_writer{std::move(writer)} {}

    void flush_buffer() {
        auto const buffer{m_serializer.get_ir_buf_view()};
        m_writer.write(reinterpret_cast<char const*>(buffer.data()), buffer.size_bytes());
        m_serializer.clear_ir_buf();
    }

    clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t> m_serializer;
    clp_s::FileWriter m_writer;
};
}  // namespace clp_s::log_converter

#endif  // CLP_S_LOG_CONVERTER_LOGSERIALIZER_HPP

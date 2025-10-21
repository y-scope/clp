#include "LogSerializer.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "../../clp/ffi/ir_stream/Serializer.hpp"
#include "../../clp/ir/types.hpp"
#include "../FileWriter.hpp"

namespace clp_s::log_converter {
auto LogSerializer::create(std::string_view output_dir, std::string_view original_file_path)
        -> std::optional<LogSerializer> {
    nlohmann::json metadata{{cOriginalFileMetadataKey, original_file_path}};
    auto serializer_result{
            clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>::create(
                    metadata
            )
    };
    if (serializer_result.has_error()) {
        return std::nullopt;
    }

    boost::uuids::random_generator uuid_generator;
    std::string const file_name{boost::uuids::to_string(uuid_generator()) + ".clp"};
    auto const converted_path{std::filesystem::path{output_dir} / file_name};
    clp_s::FileWriter writer;
    try {
        writer.open(converted_path, clp_s::FileWriter::OpenMode::CreateForWriting);
    } catch (std::exception const&) {
        return std::nullopt;
    }

    return LogSerializer{std::move(serializer_result.value()), std::move(writer)};
}

auto LogSerializer::add_message(std::string_view timestamp, std::string_view message) -> bool {
    msgpack::object_map const empty{.size = 0U, .ptr = nullptr};
    std::array<msgpack::object_kv, 2ULL> fields{
            msgpack::object_kv{
                    .key = msgpack::object{cTimestampKey},
                    .val = msgpack::object{timestamp}
            },
            msgpack::object_kv{.key = msgpack::object{cMessageKey}, .val = msgpack::object{message}}
    };
    msgpack::object_map const record{
            .size = static_cast<uint32_t>(fields.size()),
            .ptr = fields.data()
    };
    if (false == m_serializer.serialize_msgpack_map(empty, record)) {
        return false;
    }
    if (m_serializer.get_ir_buf_view().size() > cMaxIrBufSize) {
        flush_buffer();
    }
    return true;
}

auto LogSerializer::add_message(std::string_view message) -> bool {
    msgpack::object_map const empty{.size = 0U, .ptr = nullptr};
    msgpack::object_kv message_field{
            .key = msgpack::object{cMessageKey},
            .val = msgpack::object{message}
    };
    msgpack::object_map const record{.size = 1U, .ptr = &message_field};
    if (false == m_serializer.serialize_msgpack_map(empty, record)) {
        return false;
    }
    if (m_serializer.get_ir_buf_view().size() > cMaxIrBufSize) {
        flush_buffer();
    }
    return true;
}
}  // namespace clp_s::log_converter

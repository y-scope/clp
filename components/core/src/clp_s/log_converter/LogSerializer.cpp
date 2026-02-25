#include "LogSerializer.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../clp/ffi/ir_stream/Serializer.hpp"
#include "../../clp/ir/types.hpp"
#include "../FileWriter.hpp"

namespace clp_s::log_converter {
namespace {
constexpr msgpack::object_map cEmptyMap{.size = 0U, .ptr = nullptr};
}  // namespace

auto LogSerializer::create(std::string_view output_dir, std::string_view original_file_path)
        -> ystdlib::error_handling::Result<LogSerializer> {
    nlohmann::json metadata;
    metadata.emplace(cOriginalFileMetadataKey, original_file_path);
    auto serializer{YSTDLIB_ERROR_HANDLING_TRYX(
            clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>::create(
                    metadata
            )
    )};

    boost::uuids::random_generator uuid_generator;
    std::string const file_name{boost::uuids::to_string(uuid_generator()) + ".clp"};
    auto const converted_path{std::filesystem::path{output_dir} / file_name};
    clp_s::FileWriter writer;
    try {
        writer.open(converted_path, clp_s::FileWriter::OpenMode::CreateForWriting);
    } catch (std::exception const&) {
        return std::errc::no_such_file_or_directory;
    }

    return LogSerializer{std::move(serializer), std::move(writer)};
}

auto LogSerializer::add_message(std::string_view timestamp, std::string_view message)
        -> ystdlib::error_handling::Result<void> {
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
    YSTDLIB_ERROR_HANDLING_TRYV(m_serializer.serialize_msgpack_map(cEmptyMap, record));
    if (m_serializer.get_ir_buf_view().size() > cMaxIrBufSize) {
        flush_buffer();
    }
    return ystdlib::error_handling::success();
}

auto LogSerializer::add_message(std::string_view message) -> ystdlib::error_handling::Result<void> {
    msgpack::object_kv message_field{
            .key = msgpack::object{cMessageKey},
            .val = msgpack::object{message}
    };
    msgpack::object_map const record{.size = 1U, .ptr = &message_field};
    YSTDLIB_ERROR_HANDLING_TRYV(m_serializer.serialize_msgpack_map(cEmptyMap, record));
    if (m_serializer.get_ir_buf_view().size() > cMaxIrBufSize) {
        flush_buffer();
    }
    return ystdlib::error_handling::success();
}
}  // namespace clp_s::log_converter

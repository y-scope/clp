#include "utils.hpp"

#include <cstdint>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

#include "../../type_utils.hpp"
#include "protocol_constants.hpp"

namespace clp::ffi::ir_stream {
auto serialize_metadata(nlohmann::json& metadata, std::vector<int8_t>& ir_buf) -> bool {
    ir_buf.push_back(cProtocol::Metadata::EncodingJson);

    auto const metadata_serialized
            = metadata.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
    auto const metadata_serialized_length = metadata_serialized.length();
    if (metadata_serialized_length <= UINT8_MAX) {
        ir_buf.push_back(cProtocol::Metadata::LengthUByte);
        ir_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(metadata_serialized_length)));
    } else if (metadata_serialized_length <= UINT16_MAX) {
        ir_buf.push_back(cProtocol::Metadata::LengthUShort);
        serialize_int(static_cast<uint16_t>(metadata_serialized_length), ir_buf);
    } else {
        // Can't encode metadata longer than 64 KiB
        return false;
    }
    ir_buf.insert(ir_buf.cend(), metadata_serialized.cbegin(), metadata_serialized.cend());

    return true;
}
}  // namespace clp::ffi::ir_stream

#include "decoding_methods.hpp"

#include <algorithm>
#include <array>
#include <regex>
#include <string>
#include <string_view>

#include "../../ir/types.hpp"
#include "byteswap.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::epoch_time_ms_t;
using clp::ir::four_byte_encoded_variable_t;
using std::is_same_v;
using std::string;
using std::vector;

namespace clp::ffi::ir_stream {
/**
 * @tparam encoded_variable_t Type of the encoded variable
 * @param tag
 * @param is_encoded_var Returns true if tag is for an encoded variable (as opposed to a dictionary
 * variable)
 * @return Whether the tag is a variable tag
 */
template <typename encoded_variable_t>
static bool is_variable_tag(encoded_tag_t tag, bool& is_encoded_var);

/**
 * Deserializes a logtype from the given reader
 * @param reader
 * @param encoded_tag
 * @param logtype Returns the logtype
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 */
static IRErrorCode
deserialize_logtype(ReaderInterface& reader, encoded_tag_t encoded_tag, string& logtype);

/**
 * Deserializes a dictionary-type variable from the given reader
 * @param reader
 * @param encoded_tag
 * @param dict_var Returns the dictionary variable
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if input buffer doesn't contain enough data to deserialize
 */
static IRErrorCode
deserialize_dict_var(ReaderInterface& reader, encoded_tag_t encoded_tag, string& dict_var);

/**
 * Deserializes a timestamp from the given reader
 * @tparam encoded_variable_t Type of the encoded variable
 * @param reader
 * @param encoded_tag
 * @param ts Returns the timestamp delta if encoded_variable_t == four_byte_encoded_variable_t or
 * the actual timestamp if encoded_variable_t == eight_byte_encoded_variable_t
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 */
template <typename encoded_variable_t>
static IRErrorCode
deserialize_timestamp(ReaderInterface& reader, encoded_tag_t encoded_tag, epoch_time_ms_t& ts);

/**
 * Deserializes the next log event from the given reader
 * @tparam encoded_variable_t Type of the encoded variable
 * @param reader
 * @param encoded_tag
 * @param message Returns the deserialized message
 * @param timestamp Returns the timestamp delta if
 * encoded_variable_t == four_byte_encoded_variable_t or the actual timestamp if
 * encoded_variable_t == eight_byte_encoded_variable_t
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Decode_Error if the log event cannot be properly deserialized
 * @return Same as ffi::ir_stream::deserialize_log_event
 */
template <typename encoded_variable_t>
static IRErrorCode generic_deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
);

/**
 * Deserializes metadata from the given reader
 * @param reader
 * @param metadata_type Returns the type of the metadata found in the IR
 * @param metadata_pos Returns the starting position of the metadata in reader
 * @param metadata_size Returns the size of the metadata written in the IR
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 */
static IRErrorCode deserialize_metadata(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        uint16_t& metadata_size
);

template <typename encoded_variable_t>
static bool is_variable_tag(encoded_tag_t tag, bool& is_encoded_var) {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
             || is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if (tag == cProtocol::Payload::VarStrLenUByte || tag == cProtocol::Payload::VarStrLenUShort
        || tag == cProtocol::Payload::VarStrLenInt)
    {
        is_encoded_var = false;
        return true;
    }

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        if (tag == cProtocol::Payload::VarEightByteEncoding) {
            is_encoded_var = true;
            return true;
        }
    } else {
        if (tag == cProtocol::Payload::VarFourByteEncoding) {
            is_encoded_var = true;
            return true;
        }
    }
    return false;
}

static IRErrorCode
deserialize_logtype(ReaderInterface& reader, encoded_tag_t encoded_tag, string& logtype) {
    size_t logtype_length;
    if (encoded_tag == cProtocol::Payload::LogtypeStrLenUByte) {
        uint8_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        logtype_length = length;
    } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenUShort) {
        uint16_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        logtype_length = length;
    } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenInt) {
        int32_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        logtype_length = length;
    } else {
        return IRErrorCode_Corrupted_IR;
    }

    if (ErrorCode_Success != reader.try_read_string(logtype_length, logtype)) {
        return IRErrorCode_Incomplete_IR;
    }
    return IRErrorCode_Success;
}

static IRErrorCode
deserialize_dict_var(ReaderInterface& reader, encoded_tag_t encoded_tag, string& dict_var) {
    // Deserialize variable's length
    size_t var_length;
    if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
        uint8_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        var_length = length;
    } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
        uint16_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        var_length = length;
    } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
        int32_t length;
        if (false == deserialize_int(reader, length)) {
            return IRErrorCode_Incomplete_IR;
        }
        var_length = length;
    } else {
        return IRErrorCode_Corrupted_IR;
    }

    // Read the dictionary variable
    if (ErrorCode_Success != reader.try_read_string(var_length, dict_var)) {
        return IRErrorCode_Incomplete_IR;
    }

    return IRErrorCode_Success;
}

template <typename encoded_variable_t>
static IRErrorCode
deserialize_timestamp(ReaderInterface& reader, encoded_tag_t encoded_tag, epoch_time_ms_t& ts) {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
             || is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        if (cProtocol::Payload::TimestampVal != encoded_tag) {
            return IRErrorCode_Corrupted_IR;
        }
        if (false == deserialize_int(reader, ts)) {
            return IRErrorCode_Incomplete_IR;
        }
    } else {
        if (cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
            int8_t ts_delta;
            if (false == deserialize_int(reader, ts_delta)) {
                return IRErrorCode_Incomplete_IR;
            }
            ts = ts_delta;
        } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
            int16_t ts_delta;
            if (false == deserialize_int(reader, ts_delta)) {
                return IRErrorCode_Incomplete_IR;
            }
            ts = ts_delta;
        } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
            int32_t ts_delta;
            if (false == deserialize_int(reader, ts_delta)) {
                return IRErrorCode_Incomplete_IR;
            }
            ts = ts_delta;
        } else if (cProtocol::Payload::TimestampDeltaLong == encoded_tag) {
            int64_t ts_delta;
            if (false == deserialize_int(reader, ts_delta)) {
                return IRErrorCode_Incomplete_IR;
            }
            ts = ts_delta;
        } else {
            return IRErrorCode_Corrupted_IR;
        }
    }
    return IRErrorCode_Success;
}

template <typename encoded_variable_t>
static IRErrorCode generic_deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
) {
    message.clear();

    vector<encoded_variable_t> encoded_vars;
    vector<string> dict_vars;
    string logtype;
    if (auto error_code
        = deserialize_log_event(reader, encoded_tag, logtype, encoded_vars, dict_vars, timestamp);
        IRErrorCode_Success != error_code)
    {
        return error_code;
    }

    auto constant_handler = [&](string const& value, size_t begin_pos, size_t length) {
        message.append(value, begin_pos, length);
    };

    auto encoded_int_handler
            = [&](encoded_variable_t value) { message.append(decode_integer_var(value)); };

    auto encoded_float_handler = [&](encoded_variable_t encoded_float) {
        message.append(decode_float_var(encoded_float));
    };

    auto dict_var_handler = [&](string const& dict_var) { message.append(dict_var); };

    try {
        generic_decode_message<true>(
                logtype,
                encoded_vars,
                dict_vars,
                constant_handler,
                encoded_int_handler,
                encoded_float_handler,
                dict_var_handler
        );
    } catch (DecodingException const& e) {
        return IRErrorCode_Decode_Error;
    }
    return IRErrorCode_Success;
}

static IRErrorCode deserialize_metadata(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        uint16_t& metadata_size
) {
    if (ErrorCode_Success != reader.try_read_numeric_value(metadata_type)) {
        return IRErrorCode_Incomplete_IR;
    }

    // Read metadata length
    encoded_tag_t encoded_tag;
    if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
        return IRErrorCode_Incomplete_IR;
    }
    switch (encoded_tag) {
        case cProtocol::Metadata::LengthUByte:
            uint8_t ubyte_res;
            if (false == deserialize_int(reader, ubyte_res)) {
                return IRErrorCode_Incomplete_IR;
            }
            metadata_size = ubyte_res;
            break;
        case cProtocol::Metadata::LengthUShort:
            uint16_t ushort_res;
            if (false == deserialize_int(reader, ushort_res)) {
                return IRErrorCode_Incomplete_IR;
            }
            metadata_size = ushort_res;
            break;
        default:
            return IRErrorCode_Corrupted_IR;
    }
    return IRErrorCode_Success;
}

template <typename encoded_variable_t>
auto deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& logtype,
        vector<encoded_variable_t>& encoded_vars,
        vector<string>& dict_vars,
        epoch_time_ms_t& timestamp_or_timestamp_delta
) -> IRErrorCode {
    if (auto const err
        = deserialize_encoded_text_ast(reader, encoded_tag, logtype, encoded_vars, dict_vars);
        IRErrorCode_Success != err)
    {
        return err;
    }

    // NOTE: for the eight-byte encoding, the timestamp is the actual timestamp; for the four-byte
    // encoding, the timestamp is a timestamp delta
    if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
        return IRErrorCode_Incomplete_IR;
    }
    if (auto error_code = deserialize_timestamp<encoded_variable_t>(
                reader,
                encoded_tag,
                timestamp_or_timestamp_delta
        );
        IRErrorCode_Success != error_code)
    {
        return error_code;
    }
    return IRErrorCode_Success;
}

template <typename encoded_variable_t>
auto deserialize_encoded_text_ast(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> IRErrorCode {
    // Handle variables
    string var_str;
    bool is_encoded_var{false};
    while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
        if (is_encoded_var) {
            encoded_variable_t encoded_variable;
            if (false == deserialize_int(reader, encoded_variable)) {
                return IRErrorCode_Incomplete_IR;
            }
            encoded_vars.push_back(encoded_variable);
        } else {
            if (auto error_code = deserialize_dict_var(reader, encoded_tag, var_str);
                IRErrorCode_Success != error_code)
            {
                return error_code;
            }
            dict_vars.emplace_back(var_str);
        }
        if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
            return IRErrorCode_Incomplete_IR;
        }
    }

    // Handle logtype
    if (auto error_code = deserialize_logtype(reader, encoded_tag, logtype);
        IRErrorCode_Success != error_code)
    {
        return error_code;
    }

    return IRErrorCode_Success;
}

IRErrorCode get_encoding_type(ReaderInterface& reader, bool& is_four_bytes_encoding) {
    char buffer[cProtocol::MagicNumberLength];
    auto error_code = reader.try_read_exact_length(buffer, cProtocol::MagicNumberLength);
    if (error_code != ErrorCode_Success) {
        return IRErrorCode_Incomplete_IR;
    }
    if (0 == memcmp(buffer, cProtocol::FourByteEncodingMagicNumber, cProtocol::MagicNumberLength)) {
        is_four_bytes_encoding = true;
    } else if ((0
                == memcmp(
                        buffer,
                        cProtocol::EightByteEncodingMagicNumber,
                        cProtocol::MagicNumberLength
                )))
    {
        is_four_bytes_encoding = false;
    } else {
        return IRErrorCode_Corrupted_IR;
    }
    return IRErrorCode_Success;
}

IRErrorCode deserialize_tag(ReaderInterface& reader, encoded_tag_t& tag) {
    if (ErrorCode_Success != reader.try_read_numeric_value(tag)) {
        return IRErrorCode_Incomplete_IR;
    }
    return IRErrorCode_Success;
}

IRErrorCode deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        size_t& metadata_pos,
        uint16_t& metadata_size
) {
    if (auto error_code = deserialize_metadata(reader, metadata_type, metadata_size);
        error_code != IRErrorCode_Success)
    {
        return error_code;
    }
    metadata_pos = reader.get_pos();
    if (ErrorCode_Success != reader.try_seek_from_begin(metadata_pos + metadata_size)) {
        return IRErrorCode_Incomplete_IR;
    }
    return IRErrorCode_Success;
}

IRErrorCode deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        std::vector<int8_t>& metadata
) {
    uint16_t metadata_size{0};
    if (auto error_code = deserialize_metadata(reader, metadata_type, metadata_size);
        error_code != IRErrorCode_Success)
    {
        return error_code;
    }
    metadata.resize(metadata_size);
    if (ErrorCode_Success
        != reader.try_read_exact_length(
                size_checked_pointer_cast<char>(metadata.data()),
                metadata_size
        ))
    {
        return IRErrorCode_Incomplete_IR;
    }
    return IRErrorCode_Success;
}

auto validate_protocol_version(std::string_view protocol_version) -> IRProtocolErrorCode {
    // These versions are hardcoded to support the IR protocol version that predates the key-value
    // pair IR format.
    constexpr std::array<std::string_view, 3> cBackwardCompatibleVersions{
            "v0.0.0",
            "0.0.1",
            cProtocol::Metadata::LatestBackwardCompatibleVersion
    };
    if (cBackwardCompatibleVersions.cend()
        != std::ranges::find(cBackwardCompatibleVersions, protocol_version))
    {
        return IRProtocolErrorCode::BackwardCompatible;
    }

    std::regex const protocol_version_regex{
            static_cast<char const*>(cProtocol::Metadata::VersionRegex)
    };
    if (false
        == std::regex_match(
                protocol_version.begin(),
                protocol_version.end(),
                protocol_version_regex
        ))
    {
        return IRProtocolErrorCode::Invalid;
    }

    // TODO: Currently, we hardcode the supported versions. This should be removed once we
    // implement a proper version parser.
    if (cProtocol::Metadata::VersionValue == protocol_version) {
        return IRProtocolErrorCode::Supported;
    }

    return IRProtocolErrorCode::Unsupported;
}

IRErrorCode deserialize_utc_offset_change(ReaderInterface& reader, UtcOffset& utc_offset) {
    int64_t serialized_utc_offset{};
    if (false == deserialize_int(reader, serialized_utc_offset)) {
        return IRErrorCode_Incomplete_IR;
    }
    utc_offset = UtcOffset{serialized_utc_offset};
    return IRErrorCode_Success;
}

namespace four_byte_encoding {
IRErrorCode deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp_delta
) {
    return generic_deserialize_log_event<four_byte_encoded_variable_t>(
            reader,
            encoded_tag,
            message,
            timestamp_delta
    );
}
}  // namespace four_byte_encoding

namespace eight_byte_encoding {
IRErrorCode deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
) {
    return generic_deserialize_log_event<eight_byte_encoded_variable_t>(
            reader,
            encoded_tag,
            message,
            timestamp
    );
}
}  // namespace eight_byte_encoding

// Explicitly declare specializations
template auto deserialize_log_event<four_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& logtype,
        vector<four_byte_encoded_variable_t>& encoded_vars,
        vector<string>& dict_vars,
        epoch_time_ms_t& timestamp_or_timestamp_delta
) -> IRErrorCode;

template auto deserialize_log_event<eight_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& logtype,
        vector<eight_byte_encoded_variable_t>& encoded_vars,
        vector<string>& dict_vars,
        epoch_time_ms_t& timestamp_or_timestamp_delta
) -> IRErrorCode;

template auto deserialize_encoded_text_ast<four_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<four_byte_encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> IRErrorCode;

template auto deserialize_encoded_text_ast<eight_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<eight_byte_encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> IRErrorCode;
}  // namespace clp::ffi::ir_stream

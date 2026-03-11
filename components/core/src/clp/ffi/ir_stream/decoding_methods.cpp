#include "decoding_methods.hpp"

#include <algorithm>
#include <array>
#include <regex>
#include <string>
#include <string_view>

#include <boost/outcome/std_result.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../ir/types.hpp"
#include "../EncodedTextAst.hpp"
#include "../StringBlob.hpp"
#include "byteswap.hpp"
#include "IrDeserializationError.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::epoch_time_ms_t;
using clp::ir::four_byte_encoded_variable_t;
using std::is_same_v;
using std::string;
using std::vector;

namespace clp::ffi::ir_stream {
namespace {
/**
 * Deserializes a logtype from the given reader and appends it to the given string blob.
 * @param reader
 * @param encoded_tag
 * @param string_blob The string blob to append the deserialized logtype to.
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::InvalidTag if the encoded tag is invalid.
 * - IrDeserializationErrorEnum::IncompleteStream if the reader doesn't contain enough data to
 *   deserialize.
 * - Forwards `deserialize_int`'s return values on failure.
 */
[[nodiscard]] auto deserialize_and_append_logtype(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        StringBlob& string_blob
) -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes a dictionary variable from the given reader and appends it to the given string blob.
 * @param reader
 * @param encoded_tag
 * @param string_blob The string blob to append the deserialized logtype to.
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::InvalidTag if the encoded tag is invalid.
 * - IrDeserializationErrorEnum::IncompleteStream if the reader doesn't contain enough data to
 *   deserialize.
 * - Forwards `deserialize_int`'s return values on failure.
 */
[[nodiscard]] auto deserialize_and_append_dict_var(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        StringBlob& string_blob
) -> ystdlib::error_handling::Result<void>;

auto deserialize_and_append_logtype(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        StringBlob& string_blob
) -> ystdlib::error_handling::Result<void> {
    size_t logtype_length{};
    switch (encoded_tag) {
        case cProtocol::Payload::LogtypeStrLenUByte: {
            auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint8_t>(reader))};
            logtype_length = length;
            break;
        }
        case cProtocol::Payload::LogtypeStrLenUShort: {
            auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint16_t>(reader))};
            logtype_length = length;
            break;
        }
        case cProtocol::Payload::LogtypeStrLenInt: {
            // NOTE: Using `int32_t` to match `serialize_logtype`.
            auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int32_t>(reader))};
            logtype_length = length;
            break;
        }
        default:
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }

    auto const optional_error_code{string_blob.read_from(reader, logtype_length)};
    if (optional_error_code.has_value()) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
}

auto deserialize_and_append_dict_var(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        StringBlob& string_blob
) -> ystdlib::error_handling::Result<void> {
    size_t dict_var_length{};
    switch (encoded_tag) {
        case cProtocol::Payload::VarStrLenUByte: {
            dict_var_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint8_t>(reader));
            break;
        }
        case cProtocol::Payload::VarStrLenUShort: {
            dict_var_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint16_t>(reader));
            break;
        }
        case cProtocol::Payload::VarStrLenInt: {
            // NOTE: Using `int32_t` to match `DictionaryVariableHandler`.
            dict_var_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int32_t>(reader));
            break;
        }
        default:
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }

    auto const optional_error_code{string_blob.read_from(reader, dict_var_length)};
    if (optional_error_code.has_value()) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
}
}  // namespace

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
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::InvalidTag if the encoded tag is invalid.
 * - IrDeserializationErrorEnum::IncompleteStream if the reader doesn't contain enough data to
 *   deserialize.
 * - Forwards `deserialize_int`'s return values on failure.
 */
static auto deserialize_logtype(ReaderInterface& reader, encoded_tag_t encoded_tag, string& logtype)
        -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes a dictionary-type variable from the given reader
 * @param reader
 * @param encoded_tag
 * @return A result containing the dictionary variable on success, or an error code indicating the
 * failure:
 * - IrDeserializationErrorEnum::InvalidTag if the encoded tag is invalid.
 * - IrDeserializationErrorEnum::IncompleteStream if the input buffer doesn't contain enough data to
 *   deserialize.
 * - Forwards `deserialize_int`'s return values on failure.
 */
static auto deserialize_dict_var(ReaderInterface& reader, encoded_tag_t encoded_tag)
        -> ystdlib::error_handling::Result<string>;

/**
 * Deserializes a timestamp from the given reader
 * @tparam encoded_variable_t Type of the encoded variable
 * @param reader
 * @param encoded_tag
 * @param ts Returns the timestamp delta if encoded_variable_t == four_byte_encoded_variable_t or
 * the actual timestamp if encoded_variable_t == eight_byte_encoded_variable_t
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::InvalidTag if the encoded tag is invalid.
 * - IrDeserializationErrorEnum::IncompleteStream if the reader doesn't contain enough data to
 *   deserialize.
 * - Forwards `deserialize_int`'s return values on failure.
 */
template <typename encoded_variable_t>
static auto
deserialize_timestamp(ReaderInterface& reader, encoded_tag_t encoded_tag, epoch_time_ms_t& ts)
        -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes the next log event from the given reader
 * @tparam encoded_variable_t Type of the encoded variable
 * @param reader
 * @param encoded_tag
 * @param message Returns the deserialized message
 * @param timestamp Returns the timestamp delta if
 * encoded_variable_t == four_byte_encoded_variable_t or the actual timestamp if
 * encoded_variable_t == eight_byte_encoded_variable_t
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::MessageDecodingFailure if the log event cannot be properly
 *   deserialized.
 * - Forwards `deserialize_log_event`'s return values on failure.
 */
template <typename encoded_variable_t>
static auto generic_deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
) -> ystdlib::error_handling::Result<void>;

/**
 * Deserializes metadata from the given reader
 * @param reader
 * @param metadata_type Returns the type of the metadata found in the IR
 * @param metadata_size Returns the size of the metadata written in the IR
 * @return A void result on success, or an error code indicating the failure:
 * - IrDeserializationErrorEnum::IncompleteStream if reader doesn't contain enough data to
 *   deserialize.
 * - IrDeserializationErrorEnum::UnsupportedMetadataFormat if the metadata length format is
 *   unsupported.
 * - Forwards `deserialize_int`'s return values on failure.
 */
static auto
deserialize_metadata(ReaderInterface& reader, encoded_tag_t& metadata_type, uint16_t& metadata_size)
        -> ystdlib::error_handling::Result<void>;

template <typename encoded_variable_t>
static bool is_variable_tag(encoded_tag_t tag, bool& is_encoded_var) {
    static_assert(
            is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
            || is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
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

static auto deserialize_logtype(ReaderInterface& reader, encoded_tag_t encoded_tag, string& logtype)
        -> ystdlib::error_handling::Result<void> {
    size_t logtype_length;
    if (encoded_tag == cProtocol::Payload::LogtypeStrLenUByte) {
        logtype_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint8_t>(reader));
    } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenUShort) {
        logtype_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint16_t>(reader));
    } else if (encoded_tag == cProtocol::Payload::LogtypeStrLenInt) {
        logtype_length = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int32_t>(reader));
    } else {
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }

    if (ErrorCode_Success != reader.try_read_string(logtype_length, logtype)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
}

static auto deserialize_dict_var(ReaderInterface& reader, encoded_tag_t encoded_tag)
        -> ystdlib::error_handling::Result<string> {
    // Deserialize variable's length
    size_t var_length;
    if (cProtocol::Payload::VarStrLenUByte == encoded_tag) {
        auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint8_t>(reader))};
        var_length = length;
    } else if (cProtocol::Payload::VarStrLenUShort == encoded_tag) {
        auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint16_t>(reader))};
        var_length = length;
    } else if (cProtocol::Payload::VarStrLenInt == encoded_tag) {
        auto length{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int32_t>(reader))};
        var_length = length;
    } else {
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
    }

    // Read the dictionary variable
    string dict_var;
    if (ErrorCode_Success != reader.try_read_string(var_length, dict_var)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return dict_var;
}

template <typename encoded_variable_t>
static auto
deserialize_timestamp(ReaderInterface& reader, encoded_tag_t encoded_tag, epoch_time_ms_t& ts)
        -> ystdlib::error_handling::Result<void> {
    static_assert(
            is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
            || is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        if (cProtocol::Payload::TimestampVal != encoded_tag) {
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
        }
        ts = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<epoch_time_ms_t>(reader));
    } else {
        if (cProtocol::Payload::TimestampDeltaByte == encoded_tag) {
            ts = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int8_t>(reader));
        } else if (cProtocol::Payload::TimestampDeltaShort == encoded_tag) {
            ts = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int16_t>(reader));
        } else if (cProtocol::Payload::TimestampDeltaInt == encoded_tag) {
            ts = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int32_t>(reader));
        } else if (cProtocol::Payload::TimestampDeltaLong == encoded_tag) {
            ts = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int64_t>(reader));
        } else {
            return IrDeserializationError{IrDeserializationErrorEnum::InvalidTag};
        }
    }
    return ystdlib::error_handling::success();
}

template <typename encoded_variable_t>
static auto generic_deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
) -> ystdlib::error_handling::Result<void> {
    message.clear();

    vector<encoded_variable_t> encoded_vars;
    vector<string> dict_vars;
    string logtype;
    YSTDLIB_ERROR_HANDLING_TRYV(
            deserialize_log_event(reader, encoded_tag, logtype, encoded_vars, dict_vars, timestamp)
    );

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
        return IrDeserializationError{IrDeserializationErrorEnum::MessageDecodingFailure};
    }
    return ystdlib::error_handling::success();
}

static auto
deserialize_metadata(ReaderInterface& reader, encoded_tag_t& metadata_type, uint16_t& metadata_size)
        -> ystdlib::error_handling::Result<void> {
    if (ErrorCode_Success != reader.try_read_numeric_value(metadata_type)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }

    // Read metadata length
    encoded_tag_t encoded_tag;
    if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    switch (encoded_tag) {
        case cProtocol::Metadata::LengthUByte:
            metadata_size = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint8_t>(reader));
            break;
        case cProtocol::Metadata::LengthUShort:
            metadata_size = YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<uint16_t>(reader));
            break;
        default:
            return IrDeserializationError{IrDeserializationErrorEnum::UnsupportedMetadataFormat};
    }
    return ystdlib::error_handling::success();
}

template <typename encoded_variable_t>
auto deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& logtype,
        vector<encoded_variable_t>& encoded_vars,
        vector<string>& dict_vars,
        epoch_time_ms_t& timestamp_or_timestamp_delta
) -> ystdlib::error_handling::Result<void> {
    YSTDLIB_ERROR_HANDLING_TRYV(
            deserialize_encoded_text_ast(reader, encoded_tag, logtype, encoded_vars, dict_vars)
    );

    // NOTE: for the eight-byte encoding, the timestamp is the actual timestamp; for the four-byte
    // encoding, the timestamp is a timestamp delta
    if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }

    YSTDLIB_ERROR_HANDLING_TRYV(
            deserialize_timestamp<encoded_variable_t>(
                    reader,
                    encoded_tag,
                    timestamp_or_timestamp_delta
            )
    );
    return ystdlib::error_handling::success();
}

template <typename encoded_variable_t>
auto deserialize_encoded_text_ast(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> ystdlib::error_handling::Result<void> {
    // Handle variables
    bool is_encoded_var{false};
    while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
        if (is_encoded_var) {
            auto encoded_variable{
                    YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<encoded_variable_t>(reader))
            };
            encoded_vars.push_back(encoded_variable);
        } else {
            auto var_str{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_dict_var(reader, encoded_tag))};
            dict_vars.emplace_back(var_str);
        }
        if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
    }

    // Handle logtype
    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_logtype(reader, encoded_tag, logtype));

    return ystdlib::error_handling::success();
}

template <ir::EncodedVariableTypeReq encoded_variable_t>
[[nodiscard]] auto deserialize_encoded_text_ast(ReaderInterface& reader, encoded_tag_t encoded_tag)
        -> ystdlib::error_handling::Result<EncodedTextAst<encoded_variable_t>> {
    StringBlob string_blob;
    vector<encoded_variable_t> encoded_vars;
    bool is_encoded_var{};
    while (is_variable_tag<encoded_variable_t>(encoded_tag, is_encoded_var)) {
        if (is_encoded_var) {
            auto encoded_variable{
                    YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<encoded_variable_t>(reader))
            };
            encoded_vars.push_back(encoded_variable);
        } else {
            YSTDLIB_ERROR_HANDLING_TRYV(
                    deserialize_and_append_dict_var(reader, encoded_tag, string_blob)
            );
        }
        if (ErrorCode_Success != reader.try_read_numeric_value(encoded_tag)) {
            return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
        }
    }

    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_and_append_logtype(reader, encoded_tag, string_blob));

    return EncodedTextAst<encoded_variable_t>::create(
            std::move(encoded_vars),
            std::move(string_blob)
    );
}

auto get_encoding_type(ReaderInterface& reader, bool& is_four_bytes_encoding)
        -> ystdlib::error_handling::Result<void> {
    char buffer[cProtocol::MagicNumberLength];
    auto error_code = reader.try_read_exact_length(buffer, cProtocol::MagicNumberLength);
    if (error_code != ErrorCode_Success) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
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
        return IrDeserializationError{IrDeserializationErrorEnum::InvalidMagicNumber};
    }
    return ystdlib::error_handling::success();
}

auto deserialize_tag(ReaderInterface& reader) -> ystdlib::error_handling::Result<encoded_tag_t> {
    encoded_tag_t tag{};
    if (ErrorCode_Success != reader.try_read_numeric_value(tag)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return tag;
}

auto deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        size_t& metadata_pos,
        uint16_t& metadata_size
) -> ystdlib::error_handling::Result<void> {
    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_metadata(reader, metadata_type, metadata_size));
    metadata_pos = reader.get_pos();
    if (ErrorCode_Success != reader.try_seek_from_begin(metadata_pos + metadata_size)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
}

auto deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        std::vector<int8_t>& metadata
) -> ystdlib::error_handling::Result<void> {
    uint16_t metadata_size{0};
    YSTDLIB_ERROR_HANDLING_TRYV(deserialize_metadata(reader, metadata_type, metadata_size));
    metadata.resize(metadata_size);
    if (ErrorCode_Success
        != reader.try_read_exact_length(
                size_checked_pointer_cast<char>(metadata.data()),
                metadata_size
        ))
    {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return ystdlib::error_handling::success();
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

auto deserialize_utc_offset_change(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<UtcOffset> {
    auto serialized_utc_offset{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int64_t>(reader))};
    return UtcOffset{serialized_utc_offset};
}

namespace four_byte_encoding {
auto deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp_delta
) -> ystdlib::error_handling::Result<void> {
    return generic_deserialize_log_event<four_byte_encoded_variable_t>(
            reader,
            encoded_tag,
            message,
            timestamp_delta
    );
}
}  // namespace four_byte_encoding

namespace eight_byte_encoding {
auto deserialize_log_event(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& message,
        epoch_time_ms_t& timestamp
) -> ystdlib::error_handling::Result<void> {
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
) -> ystdlib::error_handling::Result<void>;

template auto deserialize_log_event<eight_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        string& logtype,
        vector<eight_byte_encoded_variable_t>& encoded_vars,
        vector<string>& dict_vars,
        epoch_time_ms_t& timestamp_or_timestamp_delta
) -> ystdlib::error_handling::Result<void>;

template auto deserialize_encoded_text_ast<four_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<four_byte_encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> ystdlib::error_handling::Result<void>;

template auto deserialize_encoded_text_ast<eight_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag,
        std::string& logtype,
        std::vector<eight_byte_encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars
) -> ystdlib::error_handling::Result<void>;

template auto deserialize_encoded_text_ast<four_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag
) -> ystdlib::error_handling::Result<EncodedTextAst<four_byte_encoded_variable_t>>;

template auto deserialize_encoded_text_ast<eight_byte_encoded_variable_t>(
        ReaderInterface& reader,
        encoded_tag_t encoded_tag
) -> ystdlib::error_handling::Result<EncodedTextAst<eight_byte_encoded_variable_t>>;
}  // namespace clp::ffi::ir_stream

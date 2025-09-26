#ifndef CLP_FFI_IR_STREAM_PROTOCOL_CONSTANTS_HPP
#define CLP_FFI_IR_STREAM_PROTOCOL_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace clp::ffi::ir_stream::cProtocol {
namespace Metadata {
constexpr int8_t EncodingJson = 0x1;
constexpr int8_t LengthUByte = 0x11;
constexpr int8_t LengthUShort = 0x12;

constexpr char VersionKey[] = "VERSION";
constexpr std::string_view VersionValue{"0.1.0"};

// This is used for the IR stream format that predates the key-value pair IR format.
constexpr std::string_view LatestBackwardCompatibleVersion{"0.0.2"};

// The following regex can be used to validate a Semantic Versioning string. The source of the
// regex can be found here: https://semver.org/
constexpr char VersionRegex[] = "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)"
                                "(?:-((?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)"
                                "(?:\\.(?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
                                "(?:\\+([0-9a-zA-Z-]+(?:\\.[0-9a-zA-Z-]+)*))?$";

constexpr char TimestampPatternKey[] = "TIMESTAMP_PATTERN";
constexpr char TimestampPatternSyntaxKey[] = "TIMESTAMP_PATTERN_SYNTAX";
constexpr char TimeZoneIdKey[] = "TZ_ID";
constexpr char ReferenceTimestampKey[] = "REFERENCE_TIMESTAMP";

constexpr char VariablesSchemaIdKey[] = "VARIABLES_SCHEMA_ID";
constexpr char VariableEncodingMethodsIdKey[] = "VARIABLE_ENCODING_METHODS_ID";

constexpr std::string_view UserDefinedMetadataKey{"USER_DEFINED_METADATA"};
}  // namespace Metadata

namespace Payload {
constexpr int8_t VarFourByteEncoding = 0x18;
constexpr int8_t VarEightByteEncoding = 0x19;

constexpr int8_t VarStrLenUByte = 0x11;
constexpr int8_t VarStrLenUShort = 0x12;
constexpr int8_t VarStrLenInt = 0x13;

constexpr int8_t LogtypeStrLenUByte = 0x21;
constexpr int8_t LogtypeStrLenUShort = 0x22;
constexpr int8_t LogtypeStrLenInt = 0x23;

constexpr int8_t TimestampVal = 0x30;
constexpr int8_t TimestampDeltaByte = 0x31;
constexpr int8_t TimestampDeltaShort = 0x32;
constexpr int8_t TimestampDeltaInt = 0x33;
constexpr int8_t TimestampDeltaLong = 0x34;

constexpr int8_t UtcOffsetChange = 0x3F;

constexpr int8_t StrLenUByte = 0x41;
constexpr int8_t StrLenUShort = 0x42;
constexpr int8_t StrLenUInt = 0x43;

constexpr int8_t ValueInt8 = 0x51;
constexpr int8_t ValueInt16 = 0x52;
constexpr int8_t ValueInt32 = 0x53;
constexpr int8_t ValueInt64 = 0x54;
constexpr int8_t ValueFloat = 0x56;
constexpr int8_t ValueTrue = 0x57;
constexpr int8_t ValueFalse = 0x58;
constexpr int8_t ValueFourByteEncodingClpStr = 0x59;
constexpr int8_t ValueEightByteEncodingClpStr = 0x5A;
constexpr int8_t ValueEmpty = 0x5E;
constexpr int8_t ValueNull = 0x5F;

constexpr int8_t EncodedSchemaTreeNodeParentIdByte = 0x60;
constexpr int8_t EncodedSchemaTreeNodeParentIdShort = 0x61;
constexpr int8_t EncodedSchemaTreeNodeParentIdInt = 0x62;

constexpr int8_t EncodedSchemaTreeNodeIdByte = 0x65;
constexpr int8_t EncodedSchemaTreeNodeIdShort = 0x66;
constexpr int8_t EncodedSchemaTreeNodeIdInt = 0x67;

constexpr int8_t SchemaTreeNodeMask = 0x70;

constexpr int8_t SchemaTreeNodeInt = 0x71;
constexpr int8_t SchemaTreeNodeFloat = 0x72;
constexpr int8_t SchemaTreeNodeBool = 0x73;
constexpr int8_t SchemaTreeNodeStr = 0x74;
constexpr int8_t SchemaTreeNodeUnstructuredArray = 0x75;
constexpr int8_t SchemaTreeNodeObj = 0x76;
}  // namespace Payload

constexpr int8_t FourByteEncodingMagicNumber[]
        = {static_cast<int8_t>(0xFD), 0x2F, static_cast<int8_t>(0xB5), 0x29};
constexpr int8_t EightByteEncodingMagicNumber[]
        = {static_cast<int8_t>(0xFD), 0x2F, static_cast<int8_t>(0xB5), 0x30};
constexpr std::enable_if<
        sizeof(EightByteEncodingMagicNumber) == sizeof(FourByteEncodingMagicNumber),
        size_t>::type MagicNumberLength
        = sizeof(EightByteEncodingMagicNumber);
constexpr int8_t Eof = 0x0;
}  // namespace clp::ffi::ir_stream::cProtocol

#endif  // CLP_FFI_IR_STREAM_PROTOCOL_CONSTANTS_HPP

#include "Serializer.hpp"

#include <cstdint>
#include <system_error>

#include <boost-outcome/include/boost/outcome/std_result.hpp>
#include <json/single_include/nlohmann/json.hpp>

#include "../../ir/types.hpp"
#include "../../time_types.hpp"
#include "../encoding_methods.hpp"
#include "encoding_methods.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;

namespace clp::ffi::ir_stream {
template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::create(
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<encoded_variable_t>> {
    static_assert(
            (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    Serializer<encoded_variable_t> serializer;
    auto& ir_buf{serializer.m_ir_buf};
    constexpr BufferView cMagicNumber{
            static_cast<int8_t const*>(
                    std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
                            ? cProtocol::EightByteEncodingMagicNumber
                            : cProtocol::FourByteEncodingMagicNumber
            ),
            cProtocol::MagicNumberLength
    };
    ir_buf.insert(ir_buf.cend(), cMagicNumber.begin(), cMagicNumber.end());

    nlohmann::json metadata;
    metadata.emplace(cProtocol::Metadata::VersionKey, cProtocol::Metadata::BetaVersionValue);
    metadata.emplace(cProtocol::Metadata::VariablesSchemaIdKey, cVariablesSchemaVersion);
    metadata.emplace(
            cProtocol::Metadata::VariableEncodingMethodsIdKey,
            cVariableEncodingMethodsVersion
    );
    if (false == serialize_metadata(metadata, ir_buf)) {
        return std::errc::protocol_error;
    }

    return std::move(serializer);
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::change_utc_offset(UtcOffset utc_offset) -> void {
    if (utc_offset != m_curr_utc_offset) {
        m_curr_utc_offset = utc_offset;
    }
    serialize_utc_offset_change(m_curr_utc_offset, m_ir_buf);
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template auto Serializer<eight_byte_encoded_variable_t>::create(
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<eight_byte_encoded_variable_t>>;
template auto Serializer<four_byte_encoded_variable_t>::create(
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<four_byte_encoded_variable_t>>;
template auto Serializer<eight_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;
template auto Serializer<four_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;
}  // namespace clp::ffi::ir_stream

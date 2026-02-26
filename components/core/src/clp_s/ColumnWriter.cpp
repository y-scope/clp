#include "ColumnWriter.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/EncodedTextAst.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/TraceableException.hpp>
#include <clp_s/ParsedMessage.hpp>
#include <clp_s/ZstdCompressor.hpp>

namespace clp_s {
size_t Int64ColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<int64_t>(value));
    return sizeof(int64_t);
}

void Int64ColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

auto DeltaEncodedInt64ColumnWriter::add_value(int64_t value) -> size_t {
    m_values.emplace_back(value - m_cur);
    m_cur = value;
    return sizeof(int64_t);
}

size_t DeltaEncodedInt64ColumnWriter::add_value(ParsedMessage::variable_t& value) {
    return add_value(std::get<int64_t>(value));
}

void DeltaEncodedInt64ColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t FloatColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<double>(value));
    return sizeof(double);
}

void FloatColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(double);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

size_t FormattedFloatColumnWriter::add_value(ParsedMessage::variable_t& value) {
    auto const& [float_value, format]{std::get<std::pair<double, float_format_t>>(value)};
    m_values.push_back(float_value);
    m_formats.push_back(format);
    return sizeof(double) + sizeof(float_format_t);
}

void FormattedFloatColumnWriter::store(ZstdCompressor& compressor) {
    assert(m_formats.size() == m_values.size());
    auto const values_size = m_values.size() * sizeof(double);
    auto const format_size = m_formats.size() * sizeof(float_format_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), values_size);
    compressor.write(reinterpret_cast<char const*>(m_formats.data()), format_size);
}

size_t DictionaryFloatColumnWriter::add_value(ParsedMessage::variable_t& value) {
    clp::variable_dictionary_id_t id{};
    m_var_dict->add_entry(std::get<std::string>(value), id);
    m_var_dict_ids.push_back(id);
    return sizeof(clp::variable_dictionary_id_t);
}

void DictionaryFloatColumnWriter::store(ZstdCompressor& compressor) {
    auto size{m_var_dict_ids.size() * sizeof(clp::variable_dictionary_id_t)};
    compressor.write(reinterpret_cast<char const*>(m_var_dict_ids.data()), size);
}

size_t BooleanColumnWriter::add_value(ParsedMessage::variable_t& value) {
    m_values.push_back(std::get<bool>(value) ? 1 : 0);
    return sizeof(uint8_t);
}

void BooleanColumnWriter::store(ZstdCompressor& compressor) {
    size_t size = m_values.size() * sizeof(uint8_t);
    compressor.write(reinterpret_cast<char const*>(m_values.data()), size);
}

auto ClpStringColumnWriter::add_value(ParsedMessage::variable_t& value) -> size_t {
    auto const offset{m_encoded_vars.size()};
    std::vector<clp::variable_dictionary_id_t> temp_var_dict_ids;
    if (std::holds_alternative<std::string>(value)) {
        clp::EncodedVariableInterpreter::encode_and_add_to_dictionary(
                std::get<std::string>(value),
                m_logtype_entry,
                *m_var_dict,
                m_encoded_vars,
                temp_var_dict_ids
        );
    } else if (std::holds_alternative<clp::ffi::EightByteEncodedTextAst>(value)) {
        auto const result{clp::EncodedVariableInterpreter::encode_and_add_to_dictionary(
                std::get<clp::ffi::EightByteEncodedTextAst>(value),
                m_logtype_entry,
                *m_var_dict,
                m_encoded_vars,
                temp_var_dict_ids
        )};
        if (result.has_error()) {
            auto const error{result.error()};
            throw clp::ffi::ir_stream::DecodingException(
                    clp::ErrorCode_Failure,
                    __FILENAME__,
                    __LINE__,
                    fmt::format("{}: {}", error.category().name(), error.message())
            );
        }
    } else {
        auto const result{clp::EncodedVariableInterpreter::encode_and_add_to_dictionary(
                std::get<clp::ffi::FourByteEncodedTextAst>(value),
                m_logtype_entry,
                *m_var_dict,
                m_encoded_vars,
                temp_var_dict_ids
        )};
        if (result.has_error()) {
            auto const error{result.error()};
            throw clp::ffi::ir_stream::DecodingException(
                    clp::ErrorCode_Failure,
                    __FILENAME__,
                    __LINE__,
                    fmt::format("{}: {}", error.category().name(), error.message())
            );
        }
    }

    clp::logtype_dictionary_id_t id{};
    m_log_dict->add_entry(m_logtype_entry, id);
    auto encoded_id{encode_log_dict_id(id, offset)};
    m_logtypes.push_back(encoded_id);
    return sizeof(int64_t) + (sizeof(int64_t) * (m_encoded_vars.size() - offset));
}

void ClpStringColumnWriter::store(ZstdCompressor& compressor) {
    size_t logtypes_size = m_logtypes.size() * sizeof(int64_t);
    compressor.write(reinterpret_cast<char const*>(m_logtypes.data()), logtypes_size);
    size_t encoded_vars_size = m_encoded_vars.size() * sizeof(int64_t);
    size_t num_encoded_vars = m_encoded_vars.size();
    compressor.write_numeric_value(num_encoded_vars);
    compressor.write(reinterpret_cast<char const*>(m_encoded_vars.data()), encoded_vars_size);
}

size_t VariableStringColumnWriter::add_value(ParsedMessage::variable_t& value) {
    clp::variable_dictionary_id_t id{};
    m_var_dict->add_entry(std::get<std::string>(value), id);
    m_var_dict_ids.push_back(id);
    return sizeof(clp::variable_dictionary_id_t);
}

void VariableStringColumnWriter::store(ZstdCompressor& compressor) {
    auto size{m_var_dict_ids.size() * sizeof(clp::variable_dictionary_id_t)};
    compressor.write(reinterpret_cast<char const*>(m_var_dict_ids.data()), size);
}

auto TimestampColumnWriter::add_value(ParsedMessage::variable_t& value) -> size_t {
    auto const [timestamp, encoding] = std::get<std::pair<epochtime_t, uint64_t>>(value);
    auto const encoded_timestamp_size{m_timestamps.add_value(timestamp)};
    m_timestamp_encodings.emplace_back(encoding);
    return encoded_timestamp_size + sizeof(uint64_t);
}

void TimestampColumnWriter::store(ZstdCompressor& compressor) {
    m_timestamps.store(compressor);
    size_t const encodings_size{m_timestamp_encodings.size() * sizeof(uint64_t)};
    compressor.write(reinterpret_cast<char const*>(m_timestamp_encodings.data()), encodings_size);
}
}  // namespace clp_s

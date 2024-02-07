#include "MySQLParamBindings.hpp"

#include <cstring>

#include "Defs.h"

namespace glt {
void MySQLParamBindings::clear() {
    m_statement_bindings.clear();
    m_statement_binding_lengths.clear();
}

void MySQLParamBindings::resize(size_t num_fields) {
    m_statement_bindings.resize(num_fields);
    m_statement_binding_lengths.resize(num_fields);
    for (size_t i = 0; i < num_fields; ++i) {
        auto& binding = m_statement_bindings[i];
        memset((void*)&binding, 0, sizeof(binding));
        binding.length = &m_statement_binding_lengths[i];
    }
}

void MySQLParamBindings::bind_int64(size_t field_index, int64_t& value) {
    if (field_index >= m_statement_bindings.size()) {
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }

    auto& binding = m_statement_bindings[field_index];
    binding.buffer_type = MYSQL_TYPE_LONGLONG;
    binding.buffer = &value;
    m_statement_binding_lengths[field_index] = sizeof(value);
}

void MySQLParamBindings::bind_uint64(size_t field_index, uint64_t& value) {
    if (field_index >= m_statement_bindings.size()) {
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }

    auto& binding = m_statement_bindings[field_index];
    binding.buffer_type = MYSQL_TYPE_LONGLONG;
    binding.buffer = &value;
    binding.is_unsigned = true;
    m_statement_binding_lengths[field_index] = sizeof(value);
}

void MySQLParamBindings::bind_varchar(size_t field_index, char const* value, size_t value_length) {
    if (field_index >= m_statement_bindings.size()) {
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }

    auto& binding = m_statement_bindings[field_index];
    binding.buffer_type = MYSQL_TYPE_STRING;
    // NOTE: binding.buffer is used for both input and output, so it is not defined as const.
    // However, MySQL shouldn't modify it when used as an input.
    binding.buffer = const_cast<void*>(reinterpret_cast<void const*>(value));
    binding.buffer_length = value_length;
    m_statement_binding_lengths[field_index] = value_length;
}
}  // namespace glt

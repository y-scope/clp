#ifndef REDUCER_RECORD_HPP
#define REDUCER_RECORD_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "RecordTypedKeyIterator.hpp"

namespace reducer {
/**
 * Base class for a record containing zero or more key-value pairs (elements). The value of each
 * element can be accessed by key through a typed accessor.
 */
class Record {
public:
    virtual ~Record() = default;

    [[nodiscard]] virtual std::string_view get_string_view([[maybe_unused]] std::string_view key
    ) const {
        return {};
    }

    [[nodiscard]] virtual int64_t get_int64_value([[maybe_unused]] std::string_view key) const {
        return 0;
    }

    [[nodiscard]] virtual double get_double_value([[maybe_unused]] std::string_view key) const {
        return 0.0;
    }

    /**
     * @return An iterator to the key and value type of each element in this record.
     */
    [[nodiscard]] virtual std::unique_ptr<RecordTypedKeyIterator> typed_key_iter() const = 0;
};

/**
 * Record implementation which exposes a single string key-value pair.
 *
 * The value associated with the key can be updated allowing this class to act as an adapter for a
 * larger set of data.
 */
class SingleStringRecordAdapter : public Record {
public:
    explicit SingleStringRecordAdapter(std::string key_name) : m_key_name{std::move(key_name)} {}

    void set_record_value(std::string_view value) { m_value = value; }

    [[nodiscard]] std::string_view get_string_view(std::string_view key) const override {
        if (key == m_key_name) {
            return m_value;
        }
        return {};
    }

    [[nodiscard]] std::unique_ptr<RecordTypedKeyIterator> typed_key_iter() const override {
        return std::make_unique<SingleTypedKeyIterator>(m_key_name, ValueType::String);
    }

private:
    std::string m_key_name;
    std::string_view m_value;
};

/**
 * Record implementation which exposes a single 64-bit integer key-value pair.
 *
 * The value associated with the key can be updated allowing this class to act as an adapter for a
 * larger set of data.
 */
class SingleInt64RecordAdapter : public Record {
public:
    explicit SingleInt64RecordAdapter(std::string key_name) : m_key_name{std::move(key_name)} {}

    void set_record_value(int64_t value) { m_value = value; }

    [[nodiscard]] int64_t get_int64_value(std::string_view key) const override {
        if (key == m_key_name) {
            return m_value;
        }
        return 0;
    }

    [[nodiscard]] std::unique_ptr<RecordTypedKeyIterator> typed_key_iter() const override {
        return std::make_unique<SingleTypedKeyIterator>(m_key_name, ValueType::Int64);
    }

private:
    std::string m_key_name;
    int64_t m_value{};
};

/**
 * Record implementation for an empty record.
 */
class EmptyRecord : public Record {
public:
    [[nodiscard]] std::unique_ptr<RecordTypedKeyIterator> typed_key_iter() const override {
        return std::make_unique<EmptyRecordTypedKeyIterator>();
    }
};
}  // namespace reducer

#endif  // REDUCER_RECORD_HPP

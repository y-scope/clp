#ifndef REDUCER_RECORDTYPEDKEYITERATOR_HPP
#define REDUCER_RECORDTYPEDKEYITERATOR_HPP

#include <string_view>

namespace reducer {
/**
 * The type of a value of an element in a record.
 */
enum class ValueType : uint8_t {
    String,
    Int64,
    Double
};

/**
 * A record element's key and value type.
 *
 * TODO: Consider what changes (if any) are necessary for this structure, and for iterating over
 * record values in general once we start supporting nested objects.
 */
class TypedRecordKey {
public:
    TypedRecordKey() = default;

    TypedRecordKey(std::string_view key, ValueType type) : m_key{key}, m_type{type} {}

    [[nodiscard]] std::string_view get_key() const { return m_key; }

    [[nodiscard]] ValueType get_type() const { return m_type; }

private:
    std::string_view m_key;
    ValueType m_type{ValueType::String};
};

/**
 * An iterator over all of the typed keys in a Record.
 */
class RecordTypedKeyIterator {
public:
    virtual ~RecordTypedKeyIterator() = default;

    /**
     * NOTE: It's the caller's responsibility to ensure that the iterator hasn't been exhausted
     * before calling this method.
     * @return The element pointed to by the iterator.
     */
    virtual TypedRecordKey get() = 0;

    /**
     * Advances the iterator to the next element.
     * NOTE: It's the caller's responsibility to ensure the iterator hasn't be exhausted before
     * calling this method.
     */
    virtual void next() = 0;

    /**
     * @return Whether the iterator has been exhausted.
     */
    virtual bool done() = 0;
};

/**
 * A RecordTypedKeyIterator over an empty Record.
 */
class EmptyRecordTypedKeyIterator : public RecordTypedKeyIterator {
    TypedRecordKey get() override { return {}; }

    void next() override {}

    bool done() override { return true; }
};

/**
 * A RecordTypedKeyIterator over a Record with a single element.
 */
class SingleTypedKeyIterator : public RecordTypedKeyIterator {
public:
    SingleTypedKeyIterator(std::string_view key, ValueType type) : m_key{key}, m_type{type} {}

    TypedRecordKey get() override { return {m_key, m_type}; }

    void next() override { m_done = true; }

    bool done() override { return m_done; }

private:
    std::string_view m_key;
    ValueType m_type;
    bool m_done{false};
};
}  // namespace reducer

#endif  // REDUCER_RECORDTYPEDKEYITERATOR_HPP

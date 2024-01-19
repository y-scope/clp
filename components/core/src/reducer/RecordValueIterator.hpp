#ifndef CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP
#define CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

#include <string_view>

namespace reducer {
enum class ValueType {
    STRING,
    INT64,
    DOUBLE
};

// TODO: consider what changes (if any) are necessary for this structure, and for iterating over
// record values in general once we start supporting nested objects.
struct TypedRecordKey {
    std::string_view key;
    ValueType type;
};

/**
 * Class which provides an iterator over all of the typed keys in a Record.
 */
class RecordValueIterator {
public:
    virtual ~RecordValueIterator() = default;
    virtual TypedRecordKey get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

/**
 * Class which provides a RecordValueIterator over an empty Record.
 */
class EmptyRecordValueIterator : public RecordValueIterator {
    virtual TypedRecordKey get() { return {}; }

    virtual void next() {}

    virtual bool done() { return true; }
};

/**
 * Class which provides a RecordValueIterator over a Record with a single key-value pair.
 */
class SingleValueIterator : public RecordValueIterator {
public:
    SingleValueIterator(std::string_view key, ValueType type) : m_key(key), m_type(type) {}

    virtual TypedRecordKey get() { return {m_key, m_type}; }

    virtual void next() { m_done = true; }

    virtual bool done() { return m_done; }

private:
    std::string_view m_key;
    ValueType m_type;
    bool m_done{false};
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

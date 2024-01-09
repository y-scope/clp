#ifndef CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP
#define CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

#include <string_view>

namespace reducer {
enum class ValueType {
    STRING,
    INT64,
    DOUBLE
};

// FIXME: can do better than this
struct TypedRecordKey {
    std::string const* key;
    ValueType type;
};

class RecordValueIterator {
public:
    virtual TypedRecordKey get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
    virtual ~RecordValueIterator() = default;
};

class EmptyRecordValueIterator : public RecordValueIterator {
    virtual TypedRecordKey get() { return {}; }

    virtual void next() {}

    virtual bool done() { return true; }
};

class SimpleSingleValueIterator : public RecordValueIterator {
public:
    SimpleSingleValueIterator(std::string const& key, ValueType type)
            : m_key(key),
              m_type(type),
              m_done(false) {}

    virtual TypedRecordKey get() { return {&m_key, m_type}; }

    virtual void next() { m_done = true; }

    virtual bool done() { return m_done; }

private:
    std::string const& m_key;
    ValueType m_type;
    bool m_done;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

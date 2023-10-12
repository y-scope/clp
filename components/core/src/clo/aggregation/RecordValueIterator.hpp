#ifndef CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP
#define CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

#include <string_view>

enum class ValueType {
    STRING,
    INT64,
    DOUBLE
};

// FIXME: can do better than this
struct TypedRecordKey {
    std::string const*key;
    ValueType type;
};

class RecordValueIterator {
public:
    virtual TypedRecordKey get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

class EmptyRecordValueIterator : public RecordValueIterator {
    virtual TypedRecordKey get() { return {}; }

    virtual void next() {}

    virtual bool done() { return true; }
};

class SimpleSingleValueIterator : public RecordValueIterator {
public:
    SimpleSingleValueIterator(std::string const&key, ValueType type)
            : key_(key),
              type_(type),
              done_(false) {}

    virtual TypedRecordKey get() { return {&key_, type_}; }

    virtual void next() { done_ = true; }

    virtual bool done() { return done_; }

private:
    std::string const& key_;
    ValueType type_;
    bool done_;
};

#endif  // CLP_AGGREGATION_RECORD_VALUE_ITERATOR_HPP

#ifndef CLP_AGGREGATION_RECORD_HPP
#define CLP_AGGREGATION_RECORD_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

//
// #include "RecordIterator.hpp"
#include "RecordValueIterator.hpp"

// typedef std::variant<std::string, double, int64_t> RecordValueType;

class Record {
public:
    virtual std::string_view get_string_value(std::string const& key) const {
        return std::string_view();
    }

    virtual int64_t get_int64_value(std::string const& key) const { return 0.0; }

    virtual double get_double_value(std::string const& key) const { return 0; }

    virtual std::unique_ptr<RecordValueIterator> value_it() const = 0;
};

class StringRecordAdapter : public Record {
public:
    StringRecordAdapter(std::string key_name) : key_name_(key_name) {}

    void set_record_value(std::string_view value) { value_ = value; }

    virtual std::string_view get_string_value(std::string const& key) const {
        if (key == key_name_) {
            return value_;
        }
        return std::string_view();
    }

    virtual std::unique_ptr<RecordValueIterator> value_it() const {
        return std::unique_ptr<RecordValueIterator>(
                new SimpleSingleValueIterator(key_name_, ValueType::STRING)
        );
    }

private:
    std::string key_name_;
    std::string_view value_;
};

class Int64RecordAdapter : public Record {
public:
    Int64RecordAdapter(std::string key_name) : key_name_(key_name) {}

    void set_record_value(int64_t value) { value_ = value; }

    virtual int64_t get_int64_value(std::string const& key) const {
        if (key == key_name_) {
            return value_;
        }
        return 0;
    }

    virtual std::unique_ptr<RecordValueIterator> value_it() const {
        return std::unique_ptr<RecordValueIterator>(
                new SimpleSingleValueIterator(key_name_, ValueType::INT64)
        );
    }

private:
    std::string key_name_;
    int64_t value_;
};

#endif  // CLP_AGGREGATION_RECORD_HPP

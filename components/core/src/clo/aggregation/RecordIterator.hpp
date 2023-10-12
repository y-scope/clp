#ifndef CLP_AGGREGATION_RECORDIT_HPP
#define CLP_AGGREGATION_RECORDIT_HPP

#include <map>
#include <utility>
#include <vector>

//
#include "Record.hpp"

class RecordIterator {
public:
    virtual Record const* get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

class SingleRecordIterator : public RecordIterator {
public:
    SingleRecordIterator(Record const& record) : record_(&record), done_(false) {}

    virtual Record const* get() { return record_; }

    virtual void next() {
        done_ = true;
        record_ = nullptr;
    }

    virtual bool done() { return done_; }

private:
    Record const* record_;
    bool done_;
};

class VectorRecordIterator : public RecordIterator {
public:
    VectorRecordIterator(std::vector<Record> const& record)
            : cur_(record.begin()),
              end_(record.end()) {}

    virtual Record const* get() { return cur_ != end_ ? &*cur_ : nullptr; }

    virtual void next() { ++cur_; }

    virtual bool done() { return cur_ == end_; }

private:
    std::vector<Record>::const_iterator cur_;
    std::vector<Record>::const_iterator end_;
};

#endif  // CLP_AGGREGATION_RECORDIT_HPP

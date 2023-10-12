#ifndef CLP_AGGREGATION_GROUPRECORDIT_HPP
#define CLP_AGGREGATION_GROUPRECORDIT_HPP

#include <map>

//
#include "RecordGroup.hpp"

class RecordGroupIterator {
public:
    virtual RecordGroup const* get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

class Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64MapRecordGroupIterator(std::map<GroupTags, int64_t> const& map, std::string key)
            : it_cur_(map.cbegin()),
              it_end_(map.cend()),
              record_(key) {
        group_.set_record(&record_);
    }

    virtual RecordGroup const* get() {
        record_.set_record_value(it_cur_->second);
        group_.set_tags(&it_cur_->first);
        return &group_;
    }

    virtual void next() { ++it_cur_; }

    virtual bool done() { return it_cur_ == it_end_; }

private:
    Int64RecordAdapter record_;
    BasicSingleRecordGroup group_;
    std::map<GroupTags, int64_t>::const_iterator it_cur_;
    std::map<GroupTags, int64_t>::const_iterator it_end_;
};

class EmptyRecordGroupIterator : public RecordGroupIterator {
public:
    virtual RecordGroup const* get() { return nullptr; }

    virtual void next() {}

    virtual bool done() { return true; }
};

#endif  // CLP_AGGREGATION_GROUPRECORDIT_HPP

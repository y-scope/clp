#ifndef CLP_AGGREGATION_RECORDGROUP_HPP
#define CLP_AGGREGATION_RECORDGROUP_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

//
#include "GroupTags.hpp"
#include "Record.hpp"
#include "RecordIterator.hpp"

class RecordGroup {
public:
    virtual std::unique_ptr<RecordIterator> record_it() const = 0;
    virtual GroupTags const& get_tags() const = 0;
};

// TODO: change these to pointer-based so we can have nullptr/default init

class BasicSingleRecordGroup : public RecordGroup {
public:
    BasicSingleRecordGroup() : tags_(nullptr), record_(nullptr) {}

    BasicSingleRecordGroup(GroupTags const* tags, Record const* record)
            : tags_(tags),
              record_(record) {}

    virtual std::unique_ptr<RecordIterator> record_it() const {
        return std::unique_ptr<RecordIterator>(new SingleRecordIterator(*record_));
    }

    virtual GroupTags const& get_tags() const { return *tags_; }

    void set_tags(GroupTags const* tags) { tags_ = tags; }

    void set_record(Record const* record) { record_ = record; }

private:
    GroupTags const* tags_;
    Record const* record_;
};

class BasicMultiRecordGroup : public RecordGroup {
public:
    BasicMultiRecordGroup() : tags_(nullptr), records_(nullptr) {}

    BasicMultiRecordGroup(GroupTags const* tags, std::vector<Record> const* records)
            : tags_(tags),
              records_(records) {}

    virtual std::unique_ptr<RecordIterator> record_it() const {
        return std::unique_ptr<RecordIterator>(new VectorRecordIterator(*records_));
    }

    virtual GroupTags const& get_tags() const { return *tags_; }

    void set_tags(GroupTags const* tags) { tags_ = tags; }

    void set_records(std::vector<Record> const* records) { records_ = records; }

private:
    GroupTags const* tags_;
    std::vector<Record> const* records_;
};

#endif  // CLP_AGGREGATION_RECORDGROUP_HPP

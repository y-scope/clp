#ifndef CLP_AGGREGATION_RECORDGROUP_HPP
#define CLP_AGGREGATION_RECORDGROUP_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "GroupTags.hpp"
#include "Record.hpp"
#include "RecordIterator.hpp"

namespace reducer {
class RecordGroup {
public:
    virtual std::unique_ptr<RecordIterator> record_it() const = 0;
    virtual GroupTags const& get_tags() const = 0;

    virtual ~RecordGroup() {}
};

// TODO: change these to pointer-based so we can have nullptr/default init

class BasicSingleRecordGroup : public RecordGroup {
public:
    BasicSingleRecordGroup() : m_tags(nullptr), m_record(nullptr) {}

    BasicSingleRecordGroup(GroupTags const* tags, Record const* record)
            : m_tags(tags),
              m_record(record) {}

    virtual std::unique_ptr<RecordIterator> record_it() const {
        return std::unique_ptr<RecordIterator>(new SingleRecordIterator(*m_record));
    }

    virtual GroupTags const& get_tags() const { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_record(Record const* record) { m_record = record; }

private:
    GroupTags const* m_tags;
    Record const* m_record;
};

class BasicMultiRecordGroup : public RecordGroup {
public:
    BasicMultiRecordGroup() : m_tags(nullptr), m_records(nullptr) {}

    BasicMultiRecordGroup(GroupTags const* tags, std::vector<Record> const* records)
            : m_tags(tags),
              m_records(records) {}

    virtual std::unique_ptr<RecordIterator> record_it() const {
        return std::unique_ptr<RecordIterator>(new VectorRecordIterator(*m_records));
    }

    virtual GroupTags const& get_tags() const { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_records(std::vector<Record> const* records) { m_records = records; }

private:
    GroupTags const* m_tags;
    std::vector<Record> const* m_records;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORDGROUP_HPP

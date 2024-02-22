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
/**
 * Class describing a list of records which have been aggregated by some GroupTags. This is the
 * main unit of computation in our reducer framework.
 */
class RecordGroup {
public:
    virtual ~RecordGroup() = default;
    [[nodiscard]] virtual std::unique_ptr<RecordIterator> record_it() const = 0;
    [[nodiscard]] virtual GroupTags const& get_tags() const = 0;
};

/**
 * RecordGroup implementation that exposes a single Record with GroupTags.
 *
 * The Record and GroupTags can be updated allowing this class to act as an adapter for a larger set
 * of data.
 */
class BasicSingleRecordGroup : public RecordGroup {
public:
    BasicSingleRecordGroup() : m_tags(nullptr), m_record(nullptr) {}

    BasicSingleRecordGroup(GroupTags const* tags, Record const* record)
            : m_tags(tags),
              m_record(record) {}

    [[nodiscard]] std::unique_ptr<RecordIterator> record_it() const override {
        return std::make_unique<SingleRecordIterator>(m_record);
    }

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_record(Record const* record) { m_record = record; }

private:
    GroupTags const* m_tags;
    Record const* m_record;
};

/**
 * RecordGroup implementation that exposes a list of Records with GroupTags.
 *
 * The Records and GroupTags can be updated allowing this class to act as an adapter for a larger
 * set of data.
 */
class BasicMultiRecordGroup : public RecordGroup {
public:
    BasicMultiRecordGroup() : m_tags(nullptr), m_records(nullptr) {}

    BasicMultiRecordGroup(GroupTags const* tags, std::vector<Record> const* records)
            : m_tags(tags),
              m_records(records) {}

    [[nodiscard]] std::unique_ptr<RecordIterator> record_it() const override {
        return std::make_unique<VectorRecordIterator>(m_records);
    }

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_records(std::vector<Record> const* records) { m_records = records; }

private:
    GroupTags const* m_tags;
    std::vector<Record> const* m_records;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORDGROUP_HPP

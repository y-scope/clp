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
 * main unit of computation in the reducer framework.
 */
class RecordGroup {
public:
    virtual ~RecordGroup() = default;
    [[nodiscard]] virtual GroupTags const& get_tags() const = 0;
    [[nodiscard]] virtual std::unique_ptr<RecordIterator> record_iter() const = 0;
};

/**
 * RecordGroup implementation that exposes a single Record with GroupTags.
 *
 * The Record and GroupTags can be updated allowing this class to act as an adapter for a larger set
 * of data.
 */
class BasicSingleRecordGroup : public RecordGroup {
public:
    BasicSingleRecordGroup() = default;

    BasicSingleRecordGroup(GroupTags const* tags, Record const* record)
            : m_tags(tags),
              m_record(record) {}

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_record(Record const* record) { m_record = record; }

    [[nodiscard]] std::unique_ptr<RecordIterator> record_iter() const override {
        return std::make_unique<SingleRecordIterator>(m_record);
    }

private:
    GroupTags const* m_tags{nullptr};
    Record const* m_record{nullptr};
};

/**
 * RecordGroup implementation that exposes a vector of Records with GroupTags.
 *
 * The Records and GroupTags can be updated allowing this class to act as an adapter for a larger
 * set of data.
 */
class BasicMultiRecordGroup : public RecordGroup {
public:
    BasicMultiRecordGroup() = default;

    BasicMultiRecordGroup(GroupTags const* tags, std::vector<Record> const* records)
            : m_tags(tags),
              m_records(records) {}

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_records(std::vector<Record> const* records) { m_records = records; }

    [[nodiscard]] std::unique_ptr<RecordIterator> record_iter() const override {
        return std::make_unique<VectorRecordIterator>(*m_records);
    }

private:
    GroupTags const* m_tags{nullptr};
    std::vector<Record> const* m_records{nullptr};
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORDGROUP_HPP

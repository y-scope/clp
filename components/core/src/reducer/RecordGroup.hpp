#ifndef REDUCER_RECORDGROUP_HPP
#define REDUCER_RECORDGROUP_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ConstRecordIterator.hpp"
#include "GroupTags.hpp"
#include "Record.hpp"

namespace reducer {
/**
 * Class describing a list of records which have been aggregated by some GroupTags. This is the
 * main unit of computation in the reducer framework.
 */
class RecordGroup {
public:
    virtual ~RecordGroup() = default;
    [[nodiscard]] virtual GroupTags const& get_tags() const = 0;
    [[nodiscard]] virtual ConstRecordIterator& record_iter() = 0;
};

/**
 * RecordGroup implementation that exposes a single Record with GroupTags.
 *
 * The Record and GroupTags can be updated allowing this class to act as an adapter for a larger set
 * of data.
 */
class SingleRecordGroup : public RecordGroup {
public:
    SingleRecordGroup(GroupTags const* tags, Record const& record)
            : m_tags{tags},
              m_iterator{record} {}

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_record(Record const& record) { m_iterator = SingleRecordIterator(record); }

    void reset_record_iterator() { m_iterator.reset(); }

    [[nodiscard]] ConstRecordIterator& record_iter() override { return m_iterator; }

private:
    GroupTags const* m_tags{nullptr};
    SingleRecordIterator m_iterator;
};

/**
 * RecordGroup implementation that exposes a collection of Records with GroupTags.
 *
 * The Records and GroupTags can be updated allowing this class to act as an adapter for a larger
 * set of data.
 */
class MultiRecordGroup : public RecordGroup {
public:
    MultiRecordGroup(GroupTags const* tags, std::vector<Record> const& records)
            : m_tags{tags},
              m_iterator{records} {}

    [[nodiscard]] GroupTags const& get_tags() const override { return *m_tags; }

    void set_tags(GroupTags const* tags) { m_tags = tags; }

    void set_records(std::vector<Record> const& records) {
        m_iterator = MultiRecordIterator(records);
    }

    [[nodiscard]] ConstRecordIterator& record_iter() override { return m_iterator; }

private:
    GroupTags const* m_tags{nullptr};
    MultiRecordIterator m_iterator;
};

/**
 * Stubbed out RecordGroup with empty GroupTags and no records.
 */
class EmptyRecordGroup : public RecordGroup {
    [[nodiscard]] GroupTags const& get_tags() const override { return m_tags; }

    [[nodiscard]] ConstRecordIterator& record_iter() override { return m_record_it; }

private:
    GroupTags m_tags;
    EmptyRecordIterator m_record_it;
};
}  // namespace reducer

#endif  // REDUCER_RECORDGROUP_HPP

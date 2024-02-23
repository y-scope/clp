#ifndef CLP_AGGREGATION_GROUPRECORDIT_HPP
#define CLP_AGGREGATION_GROUPRECORDIT_HPP

#include <map>
#include <set>
#include <utility>

#include "RecordGroup.hpp"

namespace reducer {
/**
 * Class that allows iterating over a list of RecordGroups.
 */
class RecordGroupIterator {
public:
    virtual ~RecordGroupIterator() = default;
    virtual RecordGroup const* get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

/**
 * Class which adapts a map of GroupTags to int64_t values into a RecordGroupIterator.
 */
class Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64MapRecordGroupIterator(std::map<GroupTags, int64_t> const& map, std::string key)
            : m_it_cur(map.cbegin()),
              m_it_end(map.cend()),
              m_record(std::move(key)) {
        m_group.set_record(&m_record);
    }

    RecordGroup const* get() override {
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_it_cur->first);
        return &m_group;
    }

    void next() override { ++m_it_cur; }

    bool done() override { return m_it_cur == m_it_end; }

private:
    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t>::const_iterator m_it_cur;
    std::map<GroupTags, int64_t>::const_iterator m_it_end;
};

/**
 * Class which adapts a map of int64_t to int64_t values into a RecordGroupIterator.
 */
class Int64Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64Int64MapRecordGroupIterator(std::map<int64_t, int64_t> const& map, std::string key)
            : m_it_cur(map.cbegin()),
              m_it_end(map.cend()),
              m_record(std::move(key)) {
        m_group.set_record(&m_record);
    }

    RecordGroup const* get() override {
        m_tags = {std::to_string(m_it_cur->first)};
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_tags);
        return &m_group;
    }

    void next() override { ++m_it_cur; }

    bool done() override { return m_it_cur == m_it_end; }

private:
    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    GroupTags m_tags;
    std::map<int64_t, int64_t>::const_iterator m_it_cur;
    std::map<int64_t, int64_t>::const_iterator m_it_end;
};

/**
 * Class which adapts a map of GroupTags to int64_t values into a RecordGroupIterator, and provides
 * filtering on the output.
 */
class FilteredInt64MapRecordGroupIterator : public RecordGroupIterator {
public:
    FilteredInt64MapRecordGroupIterator(
            std::map<GroupTags, int64_t> const& map,
            std::set<GroupTags> const& filter,
            std::string key
    )
            : m_results(map),
              m_it_end(map.cend()),
              m_it_cur(map.cend()),
              m_filter_cur(filter.cbegin()),
              m_filter_end(filter.cend()),
              m_record(std::move(key)) {
        m_group.set_record(&m_record);
        advance_to_next_filter();
    }

    RecordGroup const* get() override {
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_it_cur->first);
        return &m_group;
    }

    void next() override { advance_to_next_filter(); }

    bool done() override { return m_it_cur == m_it_end; }

private:
    void advance_to_next_filter() {
        if (m_filter_cur == m_filter_end) {
            m_it_cur = m_it_end;
            return;
        }

        do {
            m_it_cur = m_results.find(*m_filter_cur);
            ++m_filter_cur;
        } while (m_it_cur == m_it_end && m_filter_cur != m_filter_end);
    }

    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t> const& m_results;
    std::map<GroupTags, int64_t>::const_iterator m_it_cur;
    std::map<GroupTags, int64_t>::const_iterator m_it_end;
    std::set<GroupTags>::const_iterator m_filter_cur;
    std::set<GroupTags>::const_iterator m_filter_end;
};

/**
 * Class which provides a RecordGroupIterator over an empty RecordGroup.
 */
class EmptyRecordGroupIterator : public RecordGroupIterator {
public:
    RecordGroup const* get() override { return nullptr; }

    void next() override {}

    bool done() override { return true; }
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_GROUPRECORDIT_HPP

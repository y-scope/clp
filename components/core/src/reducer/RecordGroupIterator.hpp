#ifndef CLP_AGGREGATION_GROUPRECORDIT_HPP
#define CLP_AGGREGATION_GROUPRECORDIT_HPP

#include <map>
#include <set>

#include "RecordGroup.hpp"

namespace reducer {
class RecordGroupIterator {
public:
    virtual RecordGroup const* get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
    virtual ~RecordGroupIterator() = default;
};

class Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64MapRecordGroupIterator(std::map<GroupTags, int64_t> const& map, std::string key)
            : m_it_cur(map.cbegin()),
              m_it_end(map.cend()),
              m_record(key) {
        m_group.set_record(&m_record);
    }

    virtual RecordGroup const* get() {
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_it_cur->first);
        return &m_group;
    }

    virtual void next() { ++m_it_cur; }

    virtual bool done() { return m_it_cur == m_it_end; }

private:
    Int64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t>::const_iterator m_it_cur;
    std::map<GroupTags, int64_t>::const_iterator m_it_end;
};

class Int64Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64Int64MapRecordGroupIterator(std::map<int64_t, int64_t> const& map, std::string key)
            : m_it_cur(map.cbegin()),
              m_it_end(map.cend()),
              m_record(key) {
        m_group.set_record(&m_record);
    }

    virtual RecordGroup const* get() {
        m_tags = {std::to_string(m_it_cur->first)};
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_tags);
        return &m_group;
    }

    virtual void next() { ++m_it_cur; }

    virtual bool done() { return m_it_cur == m_it_end; }

private:
    Int64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    GroupTags m_tags;
    std::map<int64_t, int64_t>::const_iterator m_it_cur;
    std::map<int64_t, int64_t>::const_iterator m_it_end;
};

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
              m_record(key) {
        m_group.set_record(&m_record);
        advance_to_next_filter();
    }

    virtual RecordGroup const* get() {
        m_record.set_record_value(m_it_cur->second);
        m_group.set_tags(&m_it_cur->first);
        return &m_group;
    }

    virtual void next() { advance_to_next_filter(); }

    virtual bool done() { return m_it_cur == m_it_end; }

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

    Int64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t> const& m_results;
    std::map<GroupTags, int64_t>::const_iterator m_it_cur;
    std::map<GroupTags, int64_t>::const_iterator m_it_end;
    std::set<GroupTags>::const_iterator m_filter_cur;
    std::set<GroupTags>::const_iterator m_filter_end;
};

class EmptyRecordGroupIterator : public RecordGroupIterator {
public:
    virtual RecordGroup const* get() { return nullptr; }

    virtual void next() {}

    virtual bool done() { return true; }
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_GROUPRECORDIT_HPP

#ifndef CLP_AGGREGATION_GROUPRECORDIT_HPP
#define CLP_AGGREGATION_GROUPRECORDIT_HPP

#include <map>
#include <set>
#include <utility>

#include "RecordGroup.hpp"

namespace reducer {
/**
 * Iterator over a collection of RecordGroups.
 */
class RecordGroupIterator {
public:
    virtual ~RecordGroupIterator() = default;

    /**
     * NOTE: It's the caller's responsibility to ensure that the iterator hasn't been exhausted.
     * @return The RecordGroup pointed to by the iterator.
     */
    virtual RecordGroup& get() = 0;

    /**
     * Advances the iterator to the next RecordGroup.
     * NOTE: It's the caller's responsibility to ensure the iterator hasn't be exhausted.
     */
    virtual void next() = 0;

    /**
     * @return Whether the iterator has been exhausted.
     */
    virtual bool done() = 0;
};

/**
 * A RecordGroupIterator that exposes a map which maps GroupTags to int64_t values.
 */
class Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64MapRecordGroupIterator(std::map<GroupTags, int64_t> const& elements, std::string key)
            : m_elements_it{elements.cbegin()},
              m_elements_end_it{elements.cend()},
              m_record{std::move(key)},
              m_group{nullptr, m_record} {}

    RecordGroup& get() override {
        m_record.set_record_value(m_elements_it->second);
        m_group.set_tags(&m_elements_it->first);
        return m_group;
    }

    void next() override { ++m_elements_it; }

    bool done() override { return m_elements_it == m_elements_end_it; }

private:
    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t>::const_iterator m_elements_it;
    std::map<GroupTags, int64_t>::const_iterator m_elements_end_it;
};

/**
 * A RecordGroupIterator that exposes a map which maps int64_t keys to int64_t values.
 */
class Int64Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64Int64MapRecordGroupIterator(std::map<int64_t, int64_t> const& elements, std::string key)
            : m_elements_it{elements.cbegin()},
              m_elements_end_it{elements.cend()},
              m_record{std::move(key)},
              m_group(nullptr, m_record) {}

    RecordGroup& get() override {
        m_tags = {std::to_string(m_elements_it->first)};
        m_record.set_record_value(m_elements_it->second);
        m_group.set_tags(&m_tags);
        return m_group;
    }

    void next() override { ++m_elements_it; }

    bool done() override { return m_elements_it == m_elements_end_it; }

private:
    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    GroupTags m_tags;
    std::map<int64_t, int64_t>::const_iterator m_elements_it;
    std::map<int64_t, int64_t>::const_iterator m_elements_end_it;
};

/**
 * A RecordGroupIterator that exposes a map which maps GroupTags keys to int64_t values, filtered
 * by another set of GroupTags.
 */
class FilteredInt64MapRecordGroupIterator : public RecordGroupIterator {
public:
    FilteredInt64MapRecordGroupIterator(
            std::map<GroupTags, int64_t> const& elements,
            std::set<GroupTags> const& filter,
            std::string key
    )
            : m_elements{elements},
              m_elements_end_it{elements.cend()},
              m_elements_it{elements.cend()},
              m_filter_it{filter.cbegin()},
              m_filter_end_it{filter.cend()},
              m_record{std::move(key)},
              m_group(nullptr, m_record) {
        advance_to_next_filter();
    }

    RecordGroup& get() override {
        m_record.set_record_value(m_elements_it->second);
        m_group.set_tags(&m_elements_it->first);
        return m_group;
    }

    void next() override { advance_to_next_filter(); }

    bool done() override { return m_elements_it == m_elements_end_it; }

private:
    void advance_to_next_filter() {
        if (m_filter_it == m_filter_end_it) {
            m_elements_it = m_elements_end_it;
            return;
        }

        do {
            m_elements_it = m_elements.find(*m_filter_it);
            ++m_filter_it;
        } while (m_elements_it == m_elements_end_it && m_filter_it != m_filter_end_it);
    }

    SingleInt64RecordAdapter m_record;
    BasicSingleRecordGroup m_group;
    std::map<GroupTags, int64_t> const& m_elements;
    std::map<GroupTags, int64_t>::const_iterator m_elements_it;
    std::map<GroupTags, int64_t>::const_iterator m_elements_end_it;
    std::set<GroupTags>::const_iterator m_filter_it;
    std::set<GroupTags>::const_iterator m_filter_end_it;
};

/**
 * A RecordGroupIterator over an empty RecordGroup.
 */
class EmptyRecordGroupIterator : public RecordGroupIterator {
public:
    RecordGroup& get() override { return m_group; }

    void next() override {}

    bool done() override { return true; }

private:
    EmptyRecordGroup m_group;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_GROUPRECORDIT_HPP

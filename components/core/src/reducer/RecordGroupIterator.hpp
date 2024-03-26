#ifndef REDUCER_RECORDGROUPITERATOR_HPP
#define REDUCER_RECORDGROUPITERATOR_HPP

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
     * NOTE: It's the caller's responsibility to ensure that the iterator hasn't been exhausted
     * before calling this method.
     * @return The RecordGroup pointed to by the iterator.
     */
    virtual RecordGroup& get() = 0;

    /**
     * Advances the iterator to the next RecordGroup.
     * NOTE: It's the caller's responsibility to ensure the iterator hasn't be exhausted before
     * calling this method.
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
    Int64MapRecordGroupIterator(std::map<GroupTags, int64_t> const& map, std::string key)
            : m_map_it{map.cbegin()},
              m_map_end_it{map.cend()},
              m_record{std::move(key)},
              m_group{nullptr, m_record} {}

    RecordGroup& get() override {
        m_record.set_record_value(m_map_it->second);
        m_group.set_tags(&m_map_it->first);
        m_group.reset_record_iterator();
        return m_group;
    }

    void next() override { ++m_map_it; }

    bool done() override { return m_map_it == m_map_end_it; }

private:
    SingleInt64RecordAdapter m_record;
    SingleRecordGroup m_group;
    std::map<GroupTags, int64_t>::const_iterator m_map_it;
    std::map<GroupTags, int64_t>::const_iterator m_map_end_it;
};

/**
 * A RecordGroupIterator that exposes a map which maps int64_t keys to int64_t values.
 */
class Int64Int64MapRecordGroupIterator : public RecordGroupIterator {
public:
    Int64Int64MapRecordGroupIterator(std::map<int64_t, int64_t> const& map, std::string key)
            : m_map_it{map.cbegin()},
              m_map_end_it{map.cend()},
              m_record{std::move(key)},
              m_group(nullptr, m_record) {}

    RecordGroup& get() override {
        m_tags = {std::to_string(m_map_it->first)};
        m_record.set_record_value(m_map_it->second);
        m_group.set_tags(&m_tags);
        m_group.reset_record_iterator();
        return m_group;
    }

    void next() override { ++m_map_it; }

    bool done() override { return m_map_it == m_map_end_it; }

private:
    SingleInt64RecordAdapter m_record;
    SingleRecordGroup m_group;
    GroupTags m_tags;
    std::map<int64_t, int64_t>::const_iterator m_map_it;
    std::map<int64_t, int64_t>::const_iterator m_map_end_it;
};

/**
 * A RecordGroupIterator that exposes a map which maps GroupTags keys to int64_t values, filtered
 * by another set of GroupTags.
 */
class FilteredInt64MapRecordGroupIterator : public RecordGroupIterator {
public:
    FilteredInt64MapRecordGroupIterator(
            std::map<GroupTags, int64_t> const& map,
            std::set<GroupTags> const& filter,
            std::string key
    )
            : m_map{map},
              m_map_it{map.cend()},
              m_map_end_it{map.cend()},
              m_filter_it{filter.cbegin()},
              m_filter_end_it{filter.cend()},
              m_record{std::move(key)},
              m_group(nullptr, m_record) {
        advance_to_next_filter();
    }

    // Disable copy and move construction/assignment since m_map is a reference
    FilteredInt64MapRecordGroupIterator(FilteredInt64MapRecordGroupIterator const&) = delete;
    FilteredInt64MapRecordGroupIterator(FilteredInt64MapRecordGroupIterator const&&) = delete;
    FilteredInt64MapRecordGroupIterator const& operator=(FilteredInt64MapRecordGroupIterator const&)
            = delete;
    FilteredInt64MapRecordGroupIterator const&
    operator=(FilteredInt64MapRecordGroupIterator const&&)
            = delete;

    RecordGroup& get() override {
        m_record.set_record_value(m_map_end_it->second);
        m_group.set_tags(&m_map_end_it->first);
        m_group.reset_record_iterator();
        return m_group;
    }

    void next() override { advance_to_next_filter(); }

    bool done() override { return m_map_end_it == m_map_it; }

private:
    void advance_to_next_filter() {
        if (m_filter_it == m_filter_end_it) {
            m_map_end_it = m_map_it;
            return;
        }

        do {
            m_map_end_it = m_map.find(*m_filter_it);
            ++m_filter_it;
        } while (m_map_end_it == m_map_it && m_filter_it != m_filter_end_it);
    }

    SingleInt64RecordAdapter m_record;
    SingleRecordGroup m_group;
    std::map<GroupTags, int64_t> const& m_map;
    std::map<GroupTags, int64_t>::const_iterator m_map_end_it;
    std::map<GroupTags, int64_t>::const_iterator m_map_it;
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

#endif  // REDUCER_RECORDGROUPITERATOR_HPP

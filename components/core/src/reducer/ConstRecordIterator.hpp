#ifndef REDUCER_CONSTRECORDITERATOR_HPP
#define REDUCER_CONSTRECORDITERATOR_HPP

#include <map>
#include <utility>
#include <vector>

#include "Record.hpp"

namespace reducer {
/**
 * A const iterator over Record objects in a RecordGroup.
 */
class ConstRecordIterator {
public:
    virtual ~ConstRecordIterator() = default;

    /**
     * NOTE: It's the caller's responsibility to ensure that the iterator hasn't been exhausted
     * before calling this method.
     * @return The record pointed to by the iterator.
     */
    [[nodiscard]] virtual Record const& get() const = 0;

    /**
     * Advances the iterator to the next record.
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
 * A ConstRecordIterator over a single record.
 */
class SingleRecordIterator : public ConstRecordIterator {
public:
    explicit SingleRecordIterator(Record const& record) : m_record{&record} {}

    [[nodiscard]] Record const& get() const override { return *m_record; }

    void next() override { m_done = true; }

    bool done() override { return m_done; }

    void reset() { m_done = false; }

private:
    Record const* m_record;
    bool m_done{false};
};

/**
 * A ConstRecordIterator over a collection of Record objects.
 */
class MultiRecordIterator : public ConstRecordIterator {
public:
    explicit MultiRecordIterator(std::vector<Record> const& records)
            : m_cur{records.cbegin()},
              m_end{records.cend()} {}

    [[nodiscard]] Record const& get() const override { return *m_cur; }

    void next() override { ++m_cur; }

    bool done() override { return m_cur == m_end; }

private:
    std::vector<Record>::const_iterator m_cur;
    std::vector<Record>::const_iterator m_end;
};

/**
 * A stubbed out ConstRecordIterator with no records.
 */
class EmptyRecordIterator : public ConstRecordIterator {
public:
    [[nodiscard]] Record const& get() const override { return m_record; }

    void next() override {}

    bool done() override { return true; }

private:
    EmptyRecord m_record;
};
}  // namespace reducer

#endif  // REDUCER_CONSTRECORDITERATOR_HPP

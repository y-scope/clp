#ifndef CLP_AGGREGATION_RECORDIT_HPP
#define CLP_AGGREGATION_RECORDIT_HPP

#include <map>
#include <utility>
#include <vector>

#include "Record.hpp"

namespace reducer {
/**
 * Class which provides an iterator over Record objects in a RecordGroup.
 */
class RecordIterator {
public:
    virtual ~RecordIterator() = default;
    virtual Record const* get() = 0;
    virtual void next() = 0;
    virtual bool done() = 0;
};

/**
 * Class which provides a RecordIterator over a single record.
 */
class SingleRecordIterator : public RecordIterator {
public:
    explicit SingleRecordIterator(Record const* record) : m_record(record) {}

    Record const* get() override { return m_record; }

    void next() override {
        m_done = true;
        m_record = nullptr;
    }

    bool done() override { return m_done; }

private:
    Record const* m_record;
    bool m_done{false};
};

/**
 * Class which provides a RecordIterator over a vector of Record objects.
 */
class VectorRecordIterator : public RecordIterator {
public:
    explicit VectorRecordIterator(std::vector<Record> const* records)
            : m_cur(records->cbegin()),
              m_end(records->cend()) {}

    Record const* get() override { return m_cur != m_end ? &*m_cur : nullptr; }

    void next() override { ++m_cur; }

    bool done() override { return m_cur == m_end; }

private:
    std::vector<Record>::const_iterator m_cur;
    std::vector<Record>::const_iterator m_end;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORDIT_HPP

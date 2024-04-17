#ifndef REDUCER_JSONARRAYRECORDITERATOR_HPP
#define REDUCER_JSONARRAYRECORDITERATOR_HPP

#include <json/single_include/nlohmann/json.hpp>

#include "ConstRecordIterator.hpp"
#include "JsonRecord.hpp"

namespace reducer {
/**
 * A ConstRecordIterator over an array of JSON objects.
 */
class JsonArrayRecordIterator : public ConstRecordIterator {
public:
    explicit JsonArrayRecordIterator(nlohmann::json::array_t json_records)
            : m_json_records(std::move(json_records)),
              m_record{m_cur_json_record},
              m_json_records_it{m_json_records.begin()} {
        if (m_json_records_it != m_json_records.end()) {
            m_cur_json_record = *m_json_records_it;
        }
    }

    [[nodiscard]] Record const& get() const override { return m_record; }

    void next() override {
        ++m_json_records_it;
        if (m_json_records_it != m_json_records.end()) {
            m_cur_json_record = *m_json_records_it;
        }
    }

    bool done() override { return m_json_records_it == m_json_records.end(); }

private:
    nlohmann::json::array_t m_json_records;
    nlohmann::json::array_t::iterator m_json_records_it;
    nlohmann::json m_cur_json_record;
    JsonRecord m_record;
};
}  // namespace reducer

#endif  // REDUCER_JSONARRAYRECORDITERATOR_HPP

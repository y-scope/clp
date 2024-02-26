#ifndef REDUCER_JSONRECORD_HPP
#define REDUCER_JSONRECORD_HPP

#include <json/single_include/nlohmann/json.hpp>

#include "Record.hpp"

namespace reducer {
/**
 * Record implementation which exposes root-level primitive elements of a JSON object.
 */
class JsonRecord : public Record {
public:
    explicit JsonRecord(nlohmann::json const& record) : m_record{&record} {}

    [[nodiscard]] std::string_view get_string_view(std::string_view key) const override {
        return (*m_record)[key].template get<std::string_view>();
    }

    [[nodiscard]] int64_t get_int64_value(std::string_view key) const override {
        return (*m_record)[key].template get<int64_t>();
    }

    [[nodiscard]] double get_double_value(std::string_view key) const override {
        return (*m_record)[key].template get<double>();
    }

    // TODO: Provide a real iterator. This is fine to omit for now since it isn't used by any
    // existing code.
    [[nodiscard]] std::unique_ptr<RecordTypedKeyIterator> typed_key_iter() const override {
        return std::make_unique<EmptyRecordTypedKeyIterator>();
    }

private:
    nlohmann::json const* m_record{nullptr};
};
}  // namespace reducer

#endif  // REDUCER_JSONRECORD_HPP

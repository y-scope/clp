#ifndef CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP
#define CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP

#include <iostream>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

#include "ConstRecordIterator.hpp"
#include "Record.hpp"
#include "RecordGroup.hpp"
#include "RecordTypedKeyIterator.hpp"

namespace reducer {
/**
 * Class which exposes the Record interface on data which had been serialized by
 * the "serialize" function declared in this file.
 */
class DeserializedRecord : public Record {
public:
    explicit DeserializedRecord(nlohmann::json const& record) : m_record{&record} {}

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

/**
 * A ConstRecordIterator over data serialized by the "serialize" function declared in this file.
 */
class DeserializedRecordIterator : public ConstRecordIterator {
public:
    explicit DeserializedRecordIterator(nlohmann::json::array_t json_records)
            : m_json_records{std::move(json_records)},
              m_json_records_it{m_json_records.begin()},
              m_cur_json_record{*m_json_records_it},
              m_record{m_cur_json_record} {}

    [[nodiscard]] Record const& get() const override { return m_record; }

    void next() override { ++m_json_records_it; }

    bool done() override { return m_json_records_it == m_json_records.end(); }

private:
    nlohmann::json::array_t m_json_records;
    nlohmann::json::array_t::iterator m_json_records_it;
    nlohmann::json m_cur_json_record;
    DeserializedRecord m_record;
};

/**
 * Class which converts serialized data into a RecordGroup and exposes iterators to the underlying
 * data.
 *
 * The serialized data comes from the "serialize" function declared in this file.
 */
class DeserializedRecordGroup : public RecordGroup {
public:
    explicit DeserializedRecordGroup(std::vector<uint8_t>& serialized_data);
    DeserializedRecordGroup(char* buf, size_t len);

    [[nodiscard]] ConstRecordIterator& record_iter() override {
        return m_record_it;
    }

    [[nodiscard]] GroupTags const& get_tags() const override {
        return m_tags;
    }

private:
    void init_tags_from_json();

    GroupTags m_tags;
    nlohmann::json m_record_group;
    DeserializedRecordIterator m_record_it;
};

std::vector<uint8_t> serialize(
        GroupTags const& tags,
        ConstRecordIterator& record_it,
        std::vector<uint8_t>(ser)(nlohmann::json const& j) = nlohmann::json::to_msgpack
);

std::vector<uint8_t> serialize_timeline(GroupTags const& tags, ConstRecordIterator& record_it);

DeserializedRecordGroup deserialize(std::vector<uint8_t>& data);

DeserializedRecordGroup deserialize(char* buf, size_t len);
}  // namespace reducer

#endif  // CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP

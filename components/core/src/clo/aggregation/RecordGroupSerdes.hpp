#ifndef CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP
#define CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP

#include <iostream>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

//
#include "Record.hpp"
#include "RecordGroup.hpp"
#include "RecordIterator.hpp"
#include "RecordValueIterator.hpp"

class DeserializedRecordGroup : public RecordGroup {
public:
    DeserializedRecordGroup(std::vector<uint8_t>& serialized_data);
    DeserializedRecordGroup(char* buf, size_t len);

    virtual std::unique_ptr<RecordIterator> record_it() const;

    virtual GroupTags const& get_tags() const;

private:
    void init_tags_from_json();
    GroupTags tags_;
    nlohmann::json record_group_;
};

class DeserializedRecord : public Record {
public:
    DeserializedRecord() : record_(nullptr) {}

    void set_record(nlohmann::json const* record) { record_ = record; }

    virtual std::string_view get_string_value(std::string const& key) const {
        return (*record_)[key].template get<std::string_view>();
    }

    virtual int64_t get_int64_value(std::string const& key) const {
        return (*record_)[key].template get<int64_t>();
    }

    virtual double get_double_value(std::string const& key) const {
        return (*record_)[key].template get<double>();
    }

    // FIXME: provide a real record value iterator
    // fine to omit for now since it isn't used by any existing code
    std::unique_ptr<RecordValueIterator> value_it() const {
        return std::unique_ptr<RecordValueIterator>(new EmptyRecordValueIterator());
    }

private:
    nlohmann::json const* record_;
};

class DeserializedRecordIterator : public RecordIterator {
public:
    DeserializedRecordIterator(nlohmann::json::array_t jarray) : jarray_(jarray) {
        it_ = jarray_.begin();
        cur_json_record_ = *it_;
        record_.set_record(&cur_json_record_);
    }

    virtual Record const* get() { return &record_; }

    virtual void next() { ++it_; }

    virtual bool done() { return it_ == jarray_.end(); }

private:
    nlohmann::json cur_json_record_;
    DeserializedRecord record_;
    nlohmann::json::array_t jarray_;
    nlohmann::json::array_t::iterator it_;
};

std::vector<uint8_t> serialize(RecordGroup const& group, std::vector<uint8_t> (ser)(const nlohmann::json &j) = nlohmann::json::to_msgpack);

DeserializedRecordGroup deserialize(std::vector<uint8_t>& data);

DeserializedRecordGroup deserialize(char* buf, size_t len);

#endif  // CLP_AGGREGATION_RECORD_GROUP_SERDES_HPP

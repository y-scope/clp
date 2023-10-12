#include "RecordGroupSerdes.hpp"

#include <iostream>

#include <json/single_include/nlohmann/json.hpp>

std::vector<uint8_t> serialize(RecordGroup const& group, std::vector<uint8_t> (ser)(const nlohmann::json &j)) {
    nlohmann::json json;
    json["group_tags"] = group.get_tags();
    auto records = nlohmann::json::array();

    for (auto it = group.record_it(); !it->done(); it->next()) {
        nlohmann::json record;
        for (auto vit = it->get()->value_it(); !vit->done(); vit->next()) {
            TypedRecordKey tk = vit->get();
            switch (tk.type) {
                case ValueType::INT64:
                    record[*tk.key] = it->get()->get_int64_value(*tk.key);
                    break;
                case ValueType::STRING:
                    record[*tk.key] = it->get()->get_string_value(*tk.key);
                case ValueType::DOUBLE:
                    record[*tk.key] = it->get()->get_double_value(*tk.key);
            }
        }
        records.push_back(std::move(record));
    }
    json["records"] = std::move(records);

    return std::move(ser(json));
}

DeserializedRecordGroup deserialize(std::vector<uint8_t>& data) {
    return DeserializedRecordGroup(data);
}

DeserializedRecordGroup deserialize(char* buf, size_t len) {
    return DeserializedRecordGroup(buf, len);
}

void DeserializedRecordGroup::init_tags_from_json() {
    auto jtags = record_group_["group_tags"].template get<nlohmann::json::array_t>();
    for (auto it = jtags.begin(); it != jtags.end(); ++it) {
        tags_.push_back(it->template get<std::string>());
    }
}

DeserializedRecordGroup::DeserializedRecordGroup(std::vector<uint8_t>& serialized_data)
        : record_group_(nlohmann::json::from_msgpack(serialized_data)) {
    init_tags_from_json();
}

DeserializedRecordGroup::DeserializedRecordGroup(char* buf, size_t len)
        : record_group_(nlohmann::json::from_msgpack<char>(buf, len)) {
    init_tags_from_json();
}

std::unique_ptr<RecordIterator> DeserializedRecordGroup::record_it() const {
    return std::unique_ptr<RecordIterator>(new DeserializedRecordIterator(
            record_group_["records"].template get<nlohmann::json::array_t>()
    ));
}

GroupTags const& DeserializedRecordGroup::get_tags() const {
    return tags_;
}

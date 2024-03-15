#ifndef CLP_S_SCHEMAREADER_HPP
#define CLP_S_SCHEMAREADER_HPP

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "ColumnReader.hpp"
#include "FileReader.hpp"
#include "JsonSerializer.hpp"
#include "SchemaTree.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
class SchemaReader;

class FilterClass {
public:
    /**
     * Initializes the filter
     * @param reader
     * @param schema_id
     * @param columns
     */
    virtual void init(
            SchemaReader* reader,
            int32_t schema_id,
            std::unordered_map<int32_t, BaseColumnReader*>& columns
    ) = 0;

    /**
     * Filters the message
     * @param cur_message
     * @param extracted_values
     * @return true if the message is accepted
     */
    virtual bool filter(
            uint64_t cur_message,
            std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
    ) = 0;
};

class SchemaReader {
public:
    struct TableMetadata {
        uint64_t num_messages;
        size_t offset;
    };

    // Constructor
    explicit SchemaReader(
            std::shared_ptr<SchemaTree> schema_tree,
            int32_t schema_id,
            uint64_t num_messages,
            bool should_marshal_records
    )
            : m_schema_id(schema_id),
              m_num_messages(num_messages),
              m_cur_message(0),
              m_timestamp_column(nullptr),
              m_get_timestamp([]() -> epochtime_t { return 0; }),
              m_global_schema_tree(std::move(schema_tree)),
              m_local_schema_tree(std::make_unique<SchemaTree>()),
              m_should_marshal_records(should_marshal_records) {}

    // Destructor
    ~SchemaReader() {
        for (auto& i : m_columns) {
            delete i;
        }
    }

    /**
     * Appends a column to the schema reader
     * @param column_reader
     */
    void append_column(BaseColumnReader* column_reader);

    /**
     * Appends a column to the schema reader
     * @param id
     */
    void append_column(int32_t id);

    /**
     * Loads the encoded messages
     * @param decompressor
     */
    void load(ZstdDecompressor& decompressor);

    /**
     * Gets next message
     * @param message
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message);

    /**
     * Gets the next message matching a filter
     * @param message
     * @param filter
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message, FilterClass* filter);

    /**
     * Gets the next message matching a filter, and its timestamp
     * @param message
     * @param timestamp
     * @param filter
     * @return true if there is a next message
     */
    bool get_next_message_with_timestamp(
            std::string& message,
            epochtime_t& timestamp,
            FilterClass* filter
    );

    /**
     * Initializes the filter
     * @param filter
     */
    void initialize_filter(FilterClass* filter);

    /**
     * Marks a column as timestamp
     * @param column_reader
     */
    void mark_column_as_timestamp(BaseColumnReader* column_reader);

    int32_t get_schema_id() const { return m_schema_id; }

private:
    /**
     * Generates a local schema tree
     * @param global_id
     */
    void generate_local_tree(int32_t global_id);

    /**
     * Generates a json template
     * @param object
     * @param id
     * @param json_pointer
     */
    void generate_json_template(int32_t id);

    /**
     * Generates a json string from the extracted values
     */
    void generate_json_string();

    int32_t m_schema_id;
    uint64_t m_num_messages;
    uint64_t m_cur_message;

    std::unordered_map<int32_t, BaseColumnReader*> m_column_map;
    std::vector<BaseColumnReader*> m_columns;
    std::vector<BaseColumnReader*> m_reordered_columns;

    BaseColumnReader* m_timestamp_column;
    std::function<epochtime_t()> m_get_timestamp;

    std::shared_ptr<SchemaTree> m_global_schema_tree;
    std::unique_ptr<SchemaTree> m_local_schema_tree;
    std::unordered_map<int32_t, int32_t> m_global_id_to_local_id;
    std::unordered_map<int32_t, int32_t> m_local_id_to_global_id;

    JsonSerializer m_json_serializer;
    bool m_should_marshal_records{true};

    std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>> m_extracted_values;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAREADER_HPP

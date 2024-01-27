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
    // Constructor
    explicit SchemaReader(std::shared_ptr<SchemaTree> schema_tree, int32_t schema_id)
            : m_num_messages(0),
              m_cur_message(0),
              m_global_schema_tree(std::move(schema_tree)),
              m_schema_id(schema_id),
              m_json_serializer(std::make_shared<JsonSerializer>()) {}

    // Destructor
    ~SchemaReader() = default;

    /**
     * Opens the scheam file
     * @param path
     */
    void open(std::string path);

    /**
     * Closes the schema file
     */
    void close();

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
     */
    void load();

    /**
     * Gets next message
     * @param message
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message);

    /**
     * Gets next message with a filter
     * @param message
     * @param filter
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message, FilterClass* filter);

    /**
     * Initializes the filter
     * @param filter
     */
    void initialize_filter(FilterClass* filter);

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
     * Gets a json pointer string
     * @param s
     * @return
     */
    static std::string get_json_pointer_string(std::string const& s);

    int32_t m_schema_id;
    std::string m_path;
    uint64_t m_num_messages;
    uint64_t m_cur_message;

    FileReader m_file_reader;
    ZstdDecompressor m_decompressor;

    std::unordered_map<int32_t, BaseColumnReader*> m_column_map;
    std::vector<BaseColumnReader*> m_columns;
    std::vector<BaseColumnReader*> m_reordered_columns;

    std::shared_ptr<SchemaTree> m_global_schema_tree;
    std::shared_ptr<SchemaTree> m_local_schema_tree;
    std::unordered_map<int32_t, int32_t> m_global_id_to_local_id;
    std::unordered_map<int32_t, int32_t> m_local_id_to_global_id;

    std::shared_ptr<JsonSerializer> m_json_serializer;

    std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>> m_extracted_values;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAREADER_HPP

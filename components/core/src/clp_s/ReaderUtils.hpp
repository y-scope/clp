#ifndef CLP_S_READERUTILS_HPP
#define CLP_S_READERUTILS_HPP

#include "DictionaryReader.hpp"
#include "Schema.hpp"
#include "SchemaReader.hpp"
#include "SchemaTree.hpp"
#include "TimestampDictionaryReader.hpp"
#include "TraceableException.hpp"

namespace clp_s {
class ReaderUtils {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    using SchemaMap = std::map<int32_t, Schema>;
    static constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    /**
     * Reads the schema tree from the given archive directory
     * @param archives_dir
     * @return The schema tree
     */
    static std::shared_ptr<SchemaTree> read_schema_tree(std::string const& archives_dir);

    /**
     * Reads the schema map from the given archive directory
     * @param archive_dir
     * @return the schema map
     */
    static std::shared_ptr<SchemaMap> read_schemas(std::string const& archives_dir);

    /**
     * Opens and gets the variable dictionary reader for the given archive path
     * @param archive_path
     * @return the variable dictionary reader
     */
    static std::shared_ptr<VariableDictionaryReader> get_variable_dictionary_reader(
            std::string const& archive_path
    );

    /**
     * Opens and gets the log type dictionary reader for the given archive path
     * @param archive_path
     * @return the log type dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_log_type_dictionary_reader(
            std::string const& archive_path
    );

    /**
     * Opens and gets the array dictionary reader for the given archive path
     * @param archive_path
     * @return the array dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_array_dictionary_reader(
            std::string const& archive_path
    );

    static std::shared_ptr<TimestampDictionaryReader> get_timestamp_dictionary_reader(
            std::string const& archive_path
    );

    /**
     * Gets the list of archives in the given archive directory
     * @param archives_dir
     * @return the list of archives
     */
    static std::vector<std::string> get_archives(std::string const& archives_dir);

private:
    /**
     * Appends a column to the given schema reader
     * @param reader
     * @param column_id
     * @param schema_tree
     * @param var_dict
     * @param log_dict
     * @param array_dict
     * @param timestamp_dict
     * @return the appended reader column
     */
    static BaseColumnReader* append_reader_column(
            SchemaReader* reader,
            int32_t column_id,
            std::shared_ptr<SchemaTree> const& schema_tree,
            std::shared_ptr<VariableDictionaryReader> const& var_dict,
            std::shared_ptr<LogTypeDictionaryReader> const& log_dict,
            std::shared_ptr<LogTypeDictionaryReader> const& array_dict,
            std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict
    );
};
}  // namespace clp_s

#endif  // CLP_S_READERUTILS_HPP

#ifndef CLP_S_READERUTILS_HPP
#define CLP_S_READERUTILS_HPP

#include <string>

#include "../clp/ReaderInterface.hpp"
#include "ArchiveReaderAdaptor.hpp"
#include "DictionaryReader.hpp"
#include "InputConfig.hpp"
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
     * Reads the schema tree from an archive
     * @param adaptor
     * @return The schema tree
     */
    static std::shared_ptr<SchemaTree> read_schema_tree(ArchiveReaderAdaptor& adaptor);

    /**
     * Reads the schema map from an archive
     * @param adaptor
     * @return the schema map
     */
    static std::shared_ptr<SchemaMap> read_schemas(ArchiveReaderAdaptor& archives_dir);

    /**
     * Gets the variable dictionary reader for an archive
     * @param adaptor
     * @return the variable dictionary reader
     */
    static std::shared_ptr<VariableDictionaryReader> get_variable_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Gets the log type dictionary reader for an archive
     * @param adaptor
     * @return the log type dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_log_type_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Gets the array dictionary reader for an archive
     * @param adaptor
     * @return the array dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_array_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Tries to open a clp::ReaderInterface using the given Path and NetworkAuthOption.
     * @param path
     * @param network_auth
     * @return the opened clp::ReaderInterface or nullptr on error
     */
    static std::shared_ptr<clp::ReaderInterface>
    try_create_reader(Path const& path, NetworkAuthOption const& network_auth);

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

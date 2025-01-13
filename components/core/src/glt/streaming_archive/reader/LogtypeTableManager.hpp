#ifndef GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLEMANAGER_HPP
#define GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLEMANAGER_HPP

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../Constants.hpp"
#include "LogtypeMetadata.hpp"
#include "LogtypeTable.hpp"

namespace glt::streaming_archive::reader {

class LogtypeTableManager {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "LogtypeTableManager operation failed";
        }
    };

    LogtypeTableManager() : m_is_open(false) {}

    /**
     * Open the concated variable segment file and metadata associated with the segment
     * @param segment_path
     */
    virtual void open(std::string const& segment_path);

    virtual void close();

    std::unordered_map<logtype_dictionary_id_t, LogtypeMetadata> const& get_metadata_map() {
        return m_logtype_table_metadata;
    }

    std::vector<logtype_dictionary_id_t> const& get_single_order() const {
        return m_logtype_table_order;
    }

    std::unordered_map<combined_table_id_t, std::vector<logtype_dictionary_id_t>> const&
    get_combined_order() const {
        return m_combined_table_order;
    }

    size_t get_combined_table_count() const { return m_combined_table_count; }

protected:
    /**
     * Tries to read the file that contains the metadata for variable segments.
     * @throw ErrorCode_Failure if fail to read the metadata file
     */
    void load_metadata();

    /**
     * Tries to read concated file that contains all variable segments.
     * @throw ErrorCode_Failure if fail to open the variable segment file
     */
    void load_variables_segment();

    bool m_is_open;
    std::string m_var_column_directory_path;
    std::unordered_map<logtype_dictionary_id_t, LogtypeMetadata> m_logtype_table_metadata;
    std::unordered_map<logtype_dictionary_id_t, CombinedMetadata> m_combined_tables_metadata;
    std::unordered_map<combined_table_id_t, CombinedTableInfo> m_combined_table_info;

    std::vector<logtype_dictionary_id_t> m_logtype_table_order;
    std::unordered_map<combined_table_id_t, std::vector<logtype_dictionary_id_t>>
            m_combined_table_order;
    size_t m_combined_table_count;
    boost::iostreams::mapped_file_source m_memory_mapped_segment_file;
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLEMANAGER_HPP

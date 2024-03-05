#ifndef GLT_STREAMING_ARCHIVE_WRITER_GLTSEGMENT_HPP
#define GLT_STREAMING_ARCHIVE_WRITER_GLTSEGMENT_HPP

// C++ libraries
#include <map>

// Project headers
#include "../../streaming_compression/passthrough/Compressor.hpp"
#include "../../streaming_compression/zstd/Compressor.hpp"
#include "../../Utils.hpp"
#include "LogtypeTable.hpp"

namespace glt::streaming_archive::writer {
class GLTSegment {
    /**
     * Class representing a GLT segment. The segment maintains a collection in-memory logtype tables
     */
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::writer::GLTSegment operation failed";
        }
    };

    class CombinedTableInfo {
    public:
        size_t m_begin_offset;  // basically, at what offset of file does the table start
        size_t m_size;  // compressed stream size.

        CombinedTableInfo(size_t begin_offset, size_t size) {
            m_begin_offset = begin_offset;
            m_size = size;
        }
    };

    // Constructors
    GLTSegment() : m_id(cInvalidSegmentId) {}

    // Destructor
    ~GLTSegment();

    /**
     * Open and create the GLT segment on disk specified by segments_dir_path and id.
     * Also sets the size threshold of combining small logtype tables
     * @param segments_dir_path
     * @param id
     * @param compression_level
     * @param threshold
     */
    void open(
            std::string const& segments_dir_path,
            segment_id_t id,
            int compression_level,
            double threshold
    );

    /**
     * Close the segment and flush all logtype tables onto the disk
     */
    void close();

    bool is_open() const;
    uint64_t get_uncompressed_size();
    size_t get_compressed_size();

    size_t append_to_segment(
            logtype_dictionary_id_t logtype_id,
            epochtime_t timestamp,
            file_id_t file_id,
            std::vector<encoded_variable_t> const& encoded_vars
    );

private:
    // Method
    void open_single_table_compressor();
    void open_combined_table_compressor();
    void open_metadata_compressor();

    /**
     * Compresses and stores all in-memory logtype tables onto the disk
     * The function calculates the total size of all logtype tables, and use the
     * threshold to decide which logtype tables should be combined into a conbined-table.
     * All logtype tables will be stored in the order of Descending size. They
     * are compressed separately but stored in a single on-disk file to minimize
     * disk-io overhead.
     */
    void compress_logtype_tables_to_disk();

    /**
     * Compresses and stores a logtype tagle with given ID as a single logtype table.
     * i.e. each variable column is compressed individually
     * @param logtype_id
     */
    void write_single_logtype(logtype_dictionary_id_t logtype_id);

    /**
     * Compresses and stores a set of small logtype table as a single combined table
     * i.e. All tables are combined and compressed together as a single compression stream.
     * Return the combined table id and size by reference.
     * @param accumulated_logtype
     * @param combined_table_id
     * @param combined_tables_info
     */
    void write_combined_logtype(
            std::vector<logtype_dictionary_id_t> const& accumulated_logtype,
            std::map<combined_table_id_t, CombinedTableInfo>& combined_tables_info
    );

    uint64_t m_uncompressed_size;
    uint64_t m_compressed_size;

    FileWriter m_metadata_writer;
    FileWriter m_logtype_table_writer;
    segment_id_t m_id;
    std::string m_segment_path;

    double m_table_threshold;
    // Use map here to ensure that the log columns will be written in ascending order (same in clg)
    // Might have a performance impact though.
    std::map<logtype_dictionary_id_t, LogtypeTable> m_logtype_variables;
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Compressor m_single_compressor;
    streaming_compression::passthrough::Compressor m_combined_compressor;
    streaming_compression::passthrough::Compressor m_metadata_compressor;
#elif USE_ZSTD_COMPRESSION
    int m_compression_level;
    streaming_compression::zstd::Compressor m_single_compressor;
    streaming_compression::zstd::Compressor m_combined_compressor;
    streaming_compression::zstd::Compressor m_metadata_compressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
};
}  // namespace glt::streaming_archive::writer

#endif  // GLT_STREAMING_ARCHIVE_WRITER_GLTSEGMENT_HPP

#ifndef STREAMING_ARCHIVE_WRITER_GLTSEGMENT_HPP
#define STREAMING_ARCHIVE_WRITER_GLTSEGMENT_HPP

#include <map>
#include "../../../Utils.hpp"
#include "LogtypeTable.hpp"
#include "../../../streaming_compression/passthrough/Compressor.hpp"
#include "../../../streaming_compression/zstd/Compressor.hpp"

namespace streaming_archive::writer {
    class GLTSegment {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::GLTSegment operation failed";
            }
        };

        class CombinedTableInfo {
        public:
            size_t m_begin_offset; // basically, at what offset of file does the table start
            size_t m_size; // compressed stream size.
            CombinedTableInfo (size_t begin_offset, size_t size) {
                m_begin_offset = begin_offset;
                m_size = size;
            }
        };

        // Constructors
        GLTSegment () : m_id(cInvalidSegmentId) {}

        // Destructor
        ~GLTSegment ();

        void open (const std::string& segments_dir_path, segment_id_t id, int compression_level,
                   double threshold);

        bool is_open () const;

        void close ();

        size_t append_var_to_segment (logtype_dictionary_id_t logtype_id, epochtime_t timestamp,
                                      file_id_t file_id,
                                      const std::vector<encoded_variable_t>& encoded_vars);

        uint64_t get_uncompressed_size ();

        size_t get_compressed_size ();

        void increment_uncompressed_size (size_t file_size);

    private:

        void open_single_table_compressor ();
        void open_combined_table_compressor ();
        void open_metadata_compressor ();

        void write_single_logtype (logtype_dictionary_id_t logtype_id,
                                   const LogtypeTable& m_logtype_variables);

        void write_accumulated_logtype (const std::vector<logtype_dictionary_id_t>& accumulated_logtype,
                                        combined_table_id_t& combined_table_id,
                                        std::map<size_t, CombinedTableInfo>& combined_tables_info);

        void compress_logtype_tables_to_disk ();

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
}

#endif //GLTSEGMENT_HPP

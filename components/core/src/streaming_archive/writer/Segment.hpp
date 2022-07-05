#ifndef STREAMING_ARCHIVE_WRITER_SEGMENT_HPP
#define STREAMING_ARCHIVE_WRITER_SEGMENT_HPP

// C++ standard libraries
#include <memory>
#include <string>

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../streaming_compression/passthrough/Compressor.hpp"
#include "../../streaming_compression/zstd/Compressor.hpp"
#include "../../TraceableException.hpp"
#include "../Constants.hpp"

namespace streaming_archive { namespace writer {
    /**
     * Class for writing segments. A segment is a container for multiple compressed buffers that itself may be further compressed and then stored on disk.
     */
    class Segment {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::Segment operation failed";
            }
        };

        // Constructors
        Segment () : m_id(cInvalidSegmentId), m_offset(0) {}

        // Destructor
        ~Segment ();

        // Methods
        /**
         * Creates a segment in the given directory
         * @param segments_dir_path
         * @param id
         * @param compression_level
         * @throw streaming_archive::writer::Segment::OperationFailed if segment wasn't closed before this call
         */
        void open (const std::string& segments_dir_path, segment_id_t id, int compression_level);
        /**
         * Closes the segment
         * @throw streaming_archive::writer::Segment::OperationFailed if compression fails
         * @throw FileWriter::OperationFailed on open, write, or close failure
         */
        void close ();

        /**
         * Appends the given buffer to the segment
         * @param buf Buffer to append
         * @param buf_len
         * @param offset Offset of the buffer in the segment
         * @throw streaming_archive::writer::Segment::OperationFailed if compression fails
         */
        void append (const char* buf, uint64_t buf_len, uint64_t& offset);

        segment_id_t get_id () const { return m_id; }
        bool is_open () const;
        uint64_t get_uncompressed_size ();
        size_t get_compressed_size () const { return m_file_writer.get_pos(); }

    private:
        // Variables
        std::string m_segment_path;
        segment_id_t m_id;

        uint64_t m_offset;      // total input bytes processed

        FileWriter m_file_writer;
#if USE_PASSTHROUGH_COMPRESSION
        streaming_compression::passthrough::Compressor m_compressor;
#elif USE_ZSTD_COMPRESSION
        streaming_compression::zstd::Compressor m_compressor;
#else
        static_assert(false, "Unsupported compression mode.");
#endif
    };
} }

#endif // STREAMING_ARCHIVE_WRITER_SEGMENT_HPP

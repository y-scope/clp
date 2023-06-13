#include "Segment.hpp"

// C standard libraries
#include <sys/stat.h>

// C++ standard libraries
#include <climits>
#include <cmath>
#include <cstring>

// Project headers
#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../spdlog_with_specializations.hpp"

using std::make_unique;
using std::string;
using std::to_string;
using std::unique_ptr;

namespace streaming_archive { namespace writer {
    Segment::~Segment () {
        if (!m_segment_path.empty()) {
            SPDLOG_ERROR("streaming_archive::writer::Segment: Segment {} not closed before being destroyed causing possible data loss", m_segment_path.c_str());
        }
    }

    void Segment::open (const string& segments_dir_path, segment_id_t id, int compression_level) {
        if (!m_segment_path.empty()) {
            throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
        }

        m_id = id;

        // Construct segment path
        m_segment_path = segments_dir_path;
        m_segment_path += std::to_string(m_id);

        m_file_writer.open(m_segment_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
#if USE_PASSTHROUGH_COMPRESSION
        m_compressor.open(m_file_writer);
#elif USE_ZSTD_COMPRESSION
        m_compressor.open(m_file_writer, compression_level);
#else
        static_assert(false, "Unsupported compression mode.");
#endif
    }

    void Segment::close () {
        m_compressor.close();
        m_file_writer.flush();
        m_file_writer.close();

        // Clear Segment
        m_offset = 0;
        m_segment_path.clear();
    }

    void Segment::append (const char* buf, const uint64_t buf_len, uint64_t& offset) {
        // Compress
        m_compressor.write(buf, buf_len);

        // Return offset and update it
        offset = m_offset;
        m_offset += buf_len;
    }

    uint64_t Segment::get_uncompressed_size () {
        return m_offset;
    }

    bool Segment::is_open () const {
        return !m_segment_path.empty();
    }
} }

#include "PackedStreamReader.hpp"

#include "../clp/BoundedReader.hpp"
#include "archive_constants.hpp"
#include "ArchiveReaderAdaptor.hpp"

namespace clp_s {

void PackedStreamReader::read_metadata(ZstdDecompressor& decompressor) {
    switch (m_state) {
        case PackedStreamReaderState::Uninitialized:
            m_state = PackedStreamReaderState::MetadataRead;
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }

    size_t num_streams;
    if (auto error = decompressor.try_read_numeric_value(num_streams); ErrorCodeSuccess != error) {
        throw OperationFailed(error, __FILE__, __LINE__);
    }
    m_stream_metadata.reserve(num_streams);

    for (size_t i = 0; i < num_streams; ++i) {
        size_t file_offset;
        size_t uncompressed_size;

        if (auto error = decompressor.try_read_numeric_value(file_offset);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILE__, __LINE__);
        }

        if (auto error = decompressor.try_read_numeric_value(uncompressed_size);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILE__, __LINE__);
        }

        m_stream_metadata.emplace_back(file_offset, uncompressed_size);
    }
}

void PackedStreamReader::open_packed_streams(std::shared_ptr<ArchiveReaderAdaptor> adaptor) {
    switch (m_state) {
        case PackedStreamReaderState::MetadataRead:
            m_state = PackedStreamReaderState::PackedStreamsOpened;
            break;
        case PackedStreamReaderState::Uninitialized:
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_adaptor = adaptor;
    m_packed_stream_reader = m_adaptor->checkout_reader_for_section(constants::cArchiveTablesFile);
    if (auto rc = m_packed_stream_reader->try_get_pos(m_begin_offset);
        clp::ErrorCode::ErrorCode_Success != rc)
    {
        throw OperationFailed(static_cast<ErrorCode>(rc), __FILE__, __LINE__);
    }
}

void PackedStreamReader::close() {
    bool needs_checkin{false};
    switch (m_state) {
        case PackedStreamReaderState::PackedStreamsOpened:
        case PackedStreamReaderState::ReadingPackedStreams:
            needs_checkin = true;
            break;
        default:
            needs_checkin = false;
            break;
    }
    if (needs_checkin) {
        m_adaptor->checkin_reader_for_section(constants::cArchiveTablesFile);
    }
    m_adaptor.reset();
    m_prev_stream_id = 0ULL;
    m_begin_offset = 0ULL;
    m_stream_metadata.clear();
    m_state = PackedStreamReaderState::Uninitialized;
}

void
PackedStreamReader::read_stream(size_t stream_id, std::shared_ptr<char[]>& buf, size_t& buf_size) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB
    if (stream_id >= m_stream_metadata.size()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILE__, __LINE__);
    }

    switch (m_state) {
        case PackedStreamReaderState::PackedStreamsOpened:
            m_state = PackedStreamReaderState::ReadingPackedStreams;
            break;
        case PackedStreamReaderState::ReadingPackedStreams:
            if (m_prev_stream_id >= stream_id) {
                throw OperationFailed(ErrorCodeBadParam, __FILE__, __LINE__);
            }
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_prev_stream_id = stream_id;

    auto& [file_offset, uncompressed_size] = m_stream_metadata[stream_id];
    size_t adjusted_file_offset = m_begin_offset + file_offset;
    if (auto error = m_packed_stream_reader->try_seek_from_begin(adjusted_file_offset);
        clp::ErrorCode::ErrorCode_Success != error)
    {
        throw OperationFailed(static_cast<ErrorCode>(error), __FILE__, __LINE__);
    }

    size_t end_pos = m_adaptor->get_header().compressed_size;
    if ((stream_id + 1) < m_stream_metadata.size()) {
        end_pos = m_begin_offset + m_stream_metadata[stream_id + 1].file_offset;
    }
    clp::BoundedReader bounded_reader{m_packed_stream_reader.get(), end_pos};

    m_packed_stream_decompressor.open(bounded_reader, cDecompressorFileReadBufferCapacity);
    if (buf_size < uncompressed_size) {
        // make_shared is supposed to work here for c++20, but it seems like the compiler version
        // we use doesn't support it, so we convert a unique_ptr to a shared_ptr instead.
        buf = std::make_unique<char[]>(uncompressed_size);
        buf_size = uncompressed_size;
    }
    if (auto error
        = m_packed_stream_decompressor.try_read_exact_length(buf.get(), uncompressed_size);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILE__, __LINE__);
    }
    m_packed_stream_decompressor.close_for_reuse();
}
}  // namespace clp_s

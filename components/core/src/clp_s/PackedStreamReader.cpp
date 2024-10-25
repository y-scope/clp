#include "PackedStreamReader.hpp"

namespace clp_s {

void PackedStreamReader::read_metadata(ZstdDecompressor& decompressor) {
    switch (m_state) {
        case PackedStreamReaderState::Uninitialized:
            m_state = PackedStreamReaderState::MetadataRead;
            break;
        case PackedStreamReaderState::PackedStreamsOpened:
            m_state = PackedStreamReaderState::PackedStreamsOpenedAndMetadataRead;
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

void PackedStreamReader::open_packed_streams(std::string const& tables_file_path) {
    switch (m_state) {
        case PackedStreamReaderState::Uninitialized:
            m_state = PackedStreamReaderState::PackedStreamsOpened;
            break;
        case PackedStreamReaderState::MetadataRead:
            m_state = PackedStreamReaderState::PackedStreamsOpenedAndMetadataRead;
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_packed_stream_reader.open(tables_file_path);
}

void PackedStreamReader::close() {
    switch (m_state) {
        case PackedStreamReaderState::PackedStreamsOpened:
        case PackedStreamReaderState::PackedStreamsOpenedAndMetadataRead:
        case PackedStreamReaderState::ReadingPackedStreams:
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_packed_stream_reader.close();
    m_prev_stream_id = 0;
    m_stream_metadata.clear();
    m_state = PackedStreamReaderState::Uninitialized;
}

void PackedStreamReader::read_stream(
        size_t stream_id,
        std::shared_ptr<char[]>& buf,
        size_t& buf_size
) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB
    if (stream_id >= m_stream_metadata.size()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILE__, __LINE__);
    }

    switch (m_state) {
        case PackedStreamReaderState::PackedStreamsOpenedAndMetadataRead:
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
    if (auto error = m_packed_stream_reader.try_seek_from_begin(file_offset);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILE__, __LINE__);
    }
    m_packed_stream_decompressor.open(m_packed_stream_reader, cDecompressorFileReadBufferCapacity);
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

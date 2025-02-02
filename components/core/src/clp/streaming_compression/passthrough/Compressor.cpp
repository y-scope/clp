#include "Compressor.hpp"

#include <cstddef>

#include "../../ErrorCode.hpp"
#include "../../TraceableException.hpp"
#include "../../WriterInterface.hpp"

namespace clp::streaming_compression::passthrough {
auto Compressor::write(char const* data, size_t const data_length) -> void {
    if (nullptr == m_compressed_stream_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (0 == data_length) {
        // Nothing needs to be done because we do not need to compress anything
        return;
    }
    if (nullptr == data) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    m_compressed_stream_writer->write(data, data_length);
}

auto Compressor::flush() -> void {
    if (nullptr == m_compressed_stream_writer) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_compressed_stream_writer->flush();
}

auto Compressor::try_get_pos(size_t& pos) const -> ErrorCode {
    if (nullptr == m_compressed_stream_writer) {
        return ErrorCode_NotInit;
    }

    return m_compressed_stream_writer->try_get_pos(pos);
}

auto Compressor::close() -> void {
    m_compressed_stream_writer = nullptr;
}

auto Compressor::open(WriterInterface& writer) -> void {
    m_compressed_stream_writer = &writer;
}
}  // namespace clp::streaming_compression::passthrough

#include "ArchiveStats.hpp"

#include <cstddef>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>

namespace clp_s {
auto LogTypeStat::compress(ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_count);
    return ystdlib::error_handling::success();
}

auto LogTypeStat::decompress(ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<LogTypeStat> {
    LogTypeStat stat{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(stat.m_count)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    return stat;
}

auto VariableStat::compress(ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_count);
    compressor.write_numeric_value(m_type.size());
    compressor.write_string(m_type);
    return ystdlib::error_handling::success();
}

auto VariableStat::decompress(ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<VariableStat> {
    VariableStat stat{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(stat.m_count)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }

    size_t type_size{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(type_size)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    if (ErrorCodeSuccess != decompressor.try_read_string(type_size, stat.m_type)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    return stat;
}
}  // namespace clp_s

#include "LogTypeStat.hpp"

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto LogTypeStat::compress(clp_s::ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_count);
    return ystdlib::error_handling::success();
}

auto LogTypeStat::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<LogTypeStat> {
    LogTypeStat stat{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(stat.m_count)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    return stat;
}
}  // namespace clpp

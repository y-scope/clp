#include "LogShapeStat.hpp"

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto LogShapeStat::compress(clp_s::ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_count);
    return ystdlib::error_handling::success();
}

auto LogShapeStat::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<LogShapeStat> {
    LogShapeStat stat{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(stat.m_count)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    return stat;
}
}  // namespace clpp

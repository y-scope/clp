#include "LogShapeStats.hpp"

#include <system_error>

#include <ystdlib/error_handling/ErrorCode.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto LogShapeStatsArray::compress(clp_s::ZstdCompressor& compressor) const -> void {
    compressor.write_numeric_value(size());
    for (auto const& stats : m_stats) {
        compressor.write_numeric_value(stats.count);
    }
}

auto LogShapeStatsArray::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    log_shape_id_t num_log_shapes{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(num_log_shapes)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    m_stats.reserve(m_stats.size() + num_log_shapes);
    for (log_shape_id_t i{0}; i < num_log_shapes; ++i) {
        LogShapeStats stats{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(stats.count)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        m_stats.emplace_back(stats);
    }
    return ystdlib::error_handling::success();
}

auto LogShapeStatsArray::add_log_shape() -> void {
    if (m_stats.size() >= cMaxLogShapeId) {
        throw std::system_error{
                ystdlib::error_handling::make_error_code(
                        ClppErrorCode{ClppErrorCodeEnum::OutOfBounds}
                ),
                "clpp::LogShapeStatsArray::add_log_shape: cMaxLogShapeId exceeded"
        };
    }
    m_stats.emplace_back();
}
}  // namespace clpp

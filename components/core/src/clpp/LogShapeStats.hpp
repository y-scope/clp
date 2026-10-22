#ifndef CLPP_LOGSHAPESTATS_HPP
#define CLPP_LOGSHAPESTATS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Defs.hpp>

namespace clpp {
/**
 * The statistics for a single log shape.
 */
struct LogShapeStats {
    uint32_t count{};
};

/**
 * The statistics of every log shape in an archive, indexed by log shape ID.
 */
class LogShapeStatsArray {
public:
    // Methods
    /**
     * Writes all stats to `compressor`.
     * @param compressor
     */
    auto compress(clp_s::ZstdCompressor& compressor) const -> void;

    /**
     * Reads stats from `decompressor`, appending to any already present.
     * @param decompressor
     * @return Success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the stream ends early or is malformed.
     */
    [[nodiscard]] auto decompress(clp_s::ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Appends zeroed stats for the next log shape ID.
     * @throws std::system_error (`ClppErrorCodeEnum::OutOfBounds`) if the number of log shapes
     * would exceed `clpp::cMaxLogShapeId`.
     */
    auto add_log_shape() -> void;

    auto increment_count(log_shape_id_t log_shape_id) -> void { ++m_stats.at(log_shape_id).count; }

    [[nodiscard]] auto get(log_shape_id_t log_shape_id) const -> LogShapeStats const& {
        return m_stats.at(log_shape_id);
    }

    [[nodiscard]] auto size() const -> log_shape_id_t {
        return static_cast<log_shape_id_t>(m_stats.size());
    }

    auto clear() -> void { m_stats.clear(); }

    /**
     * @return The number of bytes `compress` would write for the current stats.
     */
    [[nodiscard]] auto get_data_size() const -> size_t {
        return sizeof(log_shape_id_t) + (m_stats.size() * cStatsDataSize);
    }

private:
    // Constants
    static constexpr size_t cStatsDataSize{sizeof(LogShapeStats::count)};

    // Data members
    std::vector<LogShapeStats> m_stats;
};
}  // namespace clpp

#endif  // CLPP_LOGSHAPESTATS_HPP

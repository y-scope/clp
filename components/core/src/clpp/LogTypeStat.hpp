#ifndef CLPP_LOGTYPESTAT_HPP
#define CLPP_LOGTYPESTAT_HPP

#include <cstddef>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/Defs.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Array.hpp>

namespace clpp {
/*
 * Tracks the stats for a log type in an archive.
 */
class LogTypeStat {
public:
    // Methods
    [[nodiscard]] auto compress(clp_s::ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(clp_s::ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<LogTypeStat>;

    [[nodiscard]] auto get_count() const -> size_t { return m_count; }

    auto increment_count() -> void { ++m_count; }

private:
    // Data members
    size_t m_count{};
};

using LogTypeStatArray = Array<LogTypeStat, clp_s::logtype_id_t>;
}  // namespace clpp

#endif  // CLPP_LOGTYPESTAT_HPP

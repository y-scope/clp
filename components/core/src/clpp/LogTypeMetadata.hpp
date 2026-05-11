#ifndef CLPP_LOGTYPEMETADATA_HPP
#define CLPP_LOGTYPEMETADATA_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/Defs.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Array.hpp>

namespace clpp {
/*
 * Stores the metadata for a log type in an archive.
 */
class LogTypeMetadata {
public:
    // Types
    struct ParentMatchView {
        ParentMatchView(std::string_view name, size_t start, size_t size)
                : m_name(name),
                  m_start(start),
                  m_size(size) {}

        std::string m_name;
        size_t m_start;
        size_t m_size;
    };

    // Methods
    [[nodiscard]] auto compress(clp_s::ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(clp_s::ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<LogTypeMetadata>;

    template <typename... Args>
    auto emplace_parent_match(Args&&... args) -> ParentMatchView& {
        return m_parent_matches.emplace_back(std::forward<Args>(args)...);
    }

    auto parent_match(size_t i) -> ParentMatchView& { return m_parent_matches.at(i); }

    [[nodiscard]] auto get_parent_matches() const -> std::vector<ParentMatchView> const& {
        return m_parent_matches;
    }

private:
    // Data members
    std::vector<ParentMatchView> m_parent_matches;
};

using LogTypeMetadataArray = Array<LogTypeMetadata, clp_s::logtype_id_t>;
}  // namespace clpp

#endif  // CLPP_LOGTYPEMETADATA_HPP

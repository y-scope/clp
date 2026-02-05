#ifndef CLP_S_ARRAY_HPP
#define CLP_S_ARRAY_HPP

#include <cstddef>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>

namespace clp_s {
/**
 * An array that can be compressed and decompressed with Zstd. Useful for writing separate data to
 * files when a dictionary is unnecessary.
 */
template <typename Element, typename Index = size_t>
class Array {
public:
    [[nodiscard]] auto at(Index i) -> Element& { return m_array.at(i); }

    [[nodiscard]] auto at(Index i) const -> Element const& { return m_array.at(i); }

    auto at_or_create(Index id) -> Element&;

    auto at_or_create(Index id, Element e) -> Element&;

    auto clear() -> void { return m_array.clear(); }

    template <typename... Args>
    auto emplace_back(Args&&... args) -> Element& {
        return m_array.emplace_back(std::forward<Args>(args)...);
    }

    auto compress(ZstdCompressor& compressor) -> ystdlib::error_handling::Result<void>;

    auto decompress(ZstdDecompressor& decompressor) -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] auto size() const -> size_t { return m_array.size(); }

private:
    std::vector<Element> m_array;
};

template <typename Element, typename Index>
auto Array<Element, Index>::compress(ZstdCompressor& compressor)
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_array.size());
    for (auto const& stat : m_array) {
        YSTDLIB_ERROR_HANDLING_TRYV(stat.compress(compressor));
    }
    return ystdlib::error_handling::success();
}

template <typename Element, typename Index>
auto Array<Element, Index>::decompress(ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    size_t size{};
    if (ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    m_array.reserve(size);
    for (size_t i{0}; i < size; ++i) {
        m_array.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(Element::decompress(decompressor)));
    }
    return ystdlib::error_handling::success();
}

template <typename Element, typename Index>
auto Array<Element, Index>::at_or_create(Index id) -> Element& {
    return at_or_create(id, {});
}

template <typename Element, typename Index>
auto Array<Element, Index>::at_or_create(Index id, Element e) -> Element& {
    if (m_array.size() <= id) {
        m_array.resize(id + 1);
        m_array.at(id) = e;
    }
    return m_array.at(id);
}
}  // namespace clp_s
#endif  // CLP_S_ARRAY_HPP

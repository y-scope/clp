#ifndef CLPP_ARRAY_HPP
#define CLPP_ARRAY_HPP

#include <cstddef>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
/**
 * An array that can be compressed and decompressed with Zstd. Useful for writing index-based data
 * to files.
 */
template <typename Element, typename Index = size_t>
class Array {
public:
    // Methods
    [[nodiscard]] auto at(Index i) -> Element& { return m_array.at(i); }

    [[nodiscard]] auto at(Index i) const -> Element const& { return m_array.at(i); }

    auto at_or_create(Index id, Element e) -> Element&;

    auto at_or_create(Index id) -> Element& { return at_or_create(id, {}); }

    auto clear() -> void { return m_array.clear(); }

    template <typename... Args>
    auto emplace_back(Args&&... args) -> Element& {
        return m_array.emplace_back(std::forward<Args>(args)...);
    }

    auto compress(clp_s::ZstdCompressor& compressor) -> ystdlib::error_handling::Result<void>;

    auto decompress(clp_s::ZstdDecompressor& decompressor) -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] auto size() const -> size_t { return m_array.size(); }

private:
    // Data members
    std::vector<Element> m_array;
};

template <typename Element, typename Index>
auto Array<Element, Index>::compress(clp_s::ZstdCompressor& compressor)
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_array.size());
    for (auto const& element : m_array) {
        YSTDLIB_ERROR_HANDLING_TRYV(element.compress(compressor));
    }
    return ystdlib::error_handling::success();
}

template <typename Element, typename Index>
auto Array<Element, Index>::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    size_t size{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    m_array.reserve(size);
    for (size_t i{0}; i < size; ++i) {
        m_array.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(Element::decompress(decompressor)));
    }
    return ystdlib::error_handling::success();
}

template <typename Element, typename Index>
auto Array<Element, Index>::at_or_create(Index id, Element e) -> Element& {
    if (m_array.size() <= id) {
        m_array.resize(id + 1);
        m_array.at(id) = e;
    }
    return m_array.at(id);
}
}  // namespace clpp
#endif  // CLPP_ARRAY_HPP

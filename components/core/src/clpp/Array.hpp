#ifndef CLPP_ARRAY_HPP
#define CLPP_ARRAY_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/ErrorCode.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
/**
 * Constraints for `Array`'s `Index` type:
 * - An unsigned fixed-width integer from `<cstdint>`, so the element count written to disk is
 *   portable across platforms.
 * - No wider than `size_t`, so it can index a `std::vector` naturally on any platform where the
 *   format is readable.
 */
template <typename T>
concept ArrayIndexReq
        = sizeof(T) <= sizeof(size_t)
          && (std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::uint16_t>
              || std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::uint64_t>);

/**
 * An array that can be compressed and decompressed with Zstd. Useful for writing index-based data
 * to files.
 *
 * Insertion enforces that `std::numeric_limits<Index>::max()` is not exceeded, so it is safe to
 * narrow `size_t` returned by the underlying vector.
 *
 * @tparam Element The element type. Must provide `compress(ZstdCompressor&)` and
 * `static decompress(ZstdDecompressor&) -> Result<Element>` methods.
 * @tparam Index The index type, also the on-disk type for the array's size.
 */
template <typename Element, ArrayIndexReq Index>
class Array {
public:
    // Methods
    [[nodiscard]] auto at(Index i) -> Element& { return m_array.at(i); }

    [[nodiscard]] auto at(Index i) const -> Element const& { return m_array.at(i); }

    auto clear() -> void { return m_array.clear(); }

    /**
     * Throws `std::system_error` (`ClppErrorCodeEnum::OutOfBounds`) when the array already holds
     * the maximum number of elements.
     */
    template <typename... Args>
    auto emplace_back(Args&&... args) -> Element&;

    auto compress(clp_s::ZstdCompressor& compressor) -> ystdlib::error_handling::Result<void>;

    auto decompress(clp_s::ZstdDecompressor& decompressor) -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] auto size() const -> Index { return static_cast<Index>(m_array.size()); }

private:
    // Data members
    std::vector<Element> m_array;
};

template <typename Element, ArrayIndexReq Index>
template <typename... Args>
auto Array<Element, Index>::emplace_back(Args&&... args) -> Element& {
    if (size() >= std::numeric_limits<Index>::max()) {
        throw std::system_error{
                ystdlib::error_handling::make_error_code(
                        ClppErrorCode{ClppErrorCodeEnum::OutOfBounds}
                ),
                "clpp::Array::emplace_back: size would exceed the representable maximum"
        };
    }
    return m_array.emplace_back(std::forward<Args>(args)...);
}

template <typename Element, ArrayIndexReq Index>
auto Array<Element, Index>::compress(clp_s::ZstdCompressor& compressor)
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(size());
    for (auto const& element : m_array) {
        YSTDLIB_ERROR_HANDLING_TRYV(element.compress(compressor));
    }
    return ystdlib::error_handling::success();
}

template <typename Element, ArrayIndexReq Index>
auto Array<Element, Index>::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    Index size{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    m_array.reserve(size);
    for (Index i{0}; i < size; ++i) {
        m_array.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(Element::decompress(decompressor)));
    }
    return ystdlib::error_handling::success();
}
}  // namespace clpp
#endif  // CLPP_ARRAY_HPP

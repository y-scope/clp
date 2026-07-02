#ifndef CLP_S_FILTER_BITMAPVIEW_HPP
#define CLP_S_FILTER_BITMAPVIEW_HPP

#include <concepts>
#include <cstddef>
#include <system_error>

#include <ystdlib/error_handling/Result.hpp>

#include <clp/type_utils.hpp>

namespace clp_s::filter {
/**
 * Concept that defines a method that visits each set bit in a bitmap and determines whether it
 * should remain set or become unset.
 * @param bit_handler
 * @param bit_idx The index of the set bit in the bitmap.
 * @return A boolean result indicating whether the bit should remain set, or an error code
 * indicating the failure.
 */
template <typename SetBitVisitor>
concept SetBitVisitorConcept = requires(SetBitVisitor bit_handler, size_t bit_idx) {
    { bit_handler(bit_idx) } -> std::same_as<ystdlib::error_handling::Result<bool>>;
};

/**
 * Non-owning view over a bitmap backed by an external array.
 *
 * Bits are assumed to be ordered within each array element from least-significant bit to
 * most-significant bit. Bits beyond the end of the bitmap that fit into the array are expected to
 * be zero.
 */
template <clp::IntegerType BitmapComponentType>
class BitmapView {
public:
    // Static constants
    static constexpr size_t cNumBitsPerComponent{sizeof(BitmapComponentType) * 8};

    // Factory methods
    /**
     * Creates a bitmap over the given array.
     *
     * @param bitmap_array A pointer to the underlying array backing the bitmap.
     * @param num_bits The number of bits in the bitmap.
     * @return A result containing the newly created BitmapView, or an error code indicating the
     * failure:
     * - std::errc::invalid_argument if either `bitmap_array` is null or `num_bits` is zero.
     */
    [[nodiscard]] static auto create(BitmapComponentType* bitmap_array, size_t num_bits)
            -> ystdlib::error_handling::Result<BitmapView> {
        if (nullptr == bitmap_array || 0 == num_bits) {
            return std::errc::invalid_argument;
        }
        return BitmapView{bitmap_array, num_bits};
    }

    // Constructors
    // Default copy & move constructors and assignment operators
    BitmapView(BitmapView const&) = default;
    BitmapView(BitmapView&&) noexcept = default;
    auto operator=(BitmapView const&) -> BitmapView& = default;
    auto operator=(BitmapView&&) noexcept -> BitmapView& = default;

    // Methods
    /**
     * @return The number of bits in the bitmap.
     */
    [[nodiscard]] auto get_num_bits() const -> size_t { return m_num_bits; }

    /**
     * @param index
     * @return The value of the bit at the requested position, or an error code indicating the
     * failure:
     * - std::errc::result_out_of_range if the index is beyond the end of the bitmap.
     */
    [[nodiscard]] auto test_bit(size_t index) const -> ystdlib::error_handling::Result<bool> {
        if (index >= m_num_bits) {
            return std::errc::result_out_of_range;
        }

        auto const component_idx{index / cNumBitsPerComponent};
        auto const bit_offset{index % cNumBitsPerComponent};
        auto const bit_mask{static_cast<BitmapComponentType>(1) << bit_offset};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return 0 != (m_bitmap_array[component_idx] & bit_mask);
    }

    /**
     * Sets a bit at a given index to the given value.
     * @param index
     * @param value
     * @return A void result on success, or an error code indicating the failure:
     * - std::errc::result_out_of_range if the index is beyond the end of the bitmap.
     */
    [[nodiscard]] auto set_bit(size_t index, bool value) -> ystdlib::error_handling::Result<void> {
        if (index >= m_num_bits) {
            return std::errc::result_out_of_range;
        }

        auto const component_idx{index / cNumBitsPerComponent};
        auto const bit_offset{index % cNumBitsPerComponent};
        auto const bit_mask{static_cast<BitmapComponentType>(1) << bit_offset};
        if (value) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_bitmap_array[component_idx] |= bit_mask;
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_bitmap_array[component_idx] &= ~bit_mask;
        }
        return ystdlib::error_handling::success();
    }

    /**
     * Runs a visitor function over every set bit in the bitmap, modifying the state of each such
     * bit according to the return value of the visitor.
     *
     * Note: on error, some updates for set bits may be lost.
     *
     * @param set_bit_visitor
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `set_bit_visitor`'s return values on failure.
     */
    template <SetBitVisitorConcept SetBitVisitor>
    [[nodiscard]] auto filter_set_bits(SetBitVisitor set_bit_visitor)
            -> ystdlib::error_handling::Result<void> {
        for (size_t component_idx{}; component_idx < m_bitmap_array_length; ++component_idx) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (BitmapComponentType{} == m_bitmap_array[component_idx]) {
                continue;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto const current_component{m_bitmap_array[component_idx]};
            BitmapComponentType new_component{};
            auto const last_component_idx{m_bitmap_array_length - 1};
            auto const max_bit{
                    last_component_idx == component_idx
                            ? m_num_bits - (cNumBitsPerComponent * last_component_idx)
                            : cNumBitsPerComponent
            };
            for (size_t bit_idx{}; bit_idx < max_bit; ++bit_idx) {
                auto const current_bit{static_cast<BitmapComponentType>(1) << bit_idx};
                if (0 != (current_component & current_bit)) {
                    auto const current_idx{bit_idx + (cNumBitsPerComponent * component_idx)};
                    if (YSTDLIB_ERROR_HANDLING_TRYX(set_bit_visitor(current_idx))) {
                        new_component |= current_bit;
                    }
                }
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_bitmap_array[component_idx] = new_component;
        }
        return ystdlib::error_handling::success();
    }

private:
    // Constructors
    BitmapView(BitmapComponentType* bitmap_array, size_t num_bits)
            : m_bitmap_array(bitmap_array),
              m_num_bits{num_bits},
              m_bitmap_array_length{
                      (num_bits / cNumBitsPerComponent)
                      + (num_bits % cNumBitsPerComponent > 0 ? 1 : 0)
              } {}

    // Data members
    BitmapComponentType* m_bitmap_array{};
    size_t m_num_bits{};
    size_t m_bitmap_array_length{};
};
}  // namespace clp_s::filter

#endif

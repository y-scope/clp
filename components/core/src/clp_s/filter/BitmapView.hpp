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
 * A class implementing a bitmap over a view into an array.
 *
 * Bits are assumed to be ordered within each array element from least-significant bit to
 * most-significant bit. Bits beyond the end of the bitmap that fit into the array are expected to
 * be zero.
 */
template <clp::IntegerType bitmap_component_t>
class BitmapView {
public:
    static constexpr size_t cNumBitsPerComponent{sizeof(bitmap_component_t) * 8};

    [[nodiscard]] static auto create(bitmap_component_t* bitmap_array, size_t num_bits)
            -> ystdlib::error_handling::Result<BitmapView> {
        if (nullptr == bitmap_array || 0 == num_bits) {
            return std::errc::invalid_argument;
        }
        return BitmapView{bitmap_array, num_bits};
    }

    [[nodiscard]] auto get_num_bits() const -> size_t { return m_num_bits; }

    [[nodiscard]] auto test_bit(size_t index) const -> ystdlib::error_handling::Result<bool> {
        if (index >= m_num_bits) {
            return std::errc::result_out_of_range;
        }

        auto const component_idx{index / cNumBitsPerComponent};
        auto const bit_offset{index % cNumBitsPerComponent};
        auto const bit_mask{static_cast<bitmap_component_t>(1) << bit_offset};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return 0 != (m_bitmap_array[component_idx] & bit_mask);
    }

    [[nodiscard]] auto set_bit(size_t index, bool value) -> ystdlib::error_handling::Result<void> {
        if (index >= m_num_bits) {
            return std::errc::result_out_of_range;
        }

        auto const component_idx{index / cNumBitsPerComponent};
        auto const bit_offset{index % cNumBitsPerComponent};
        auto const bit_mask{static_cast<bitmap_component_t>(1) << bit_offset};
        if (value) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_bitmap_array[component_idx] |= bit_mask;
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_bitmap_array[component_idx] &= ~bit_mask;
        }
        return ystdlib::error_handling::success();
    }

    template <SetBitVisitorConcept SetBitVisitor>
    [[nodiscard]] auto filter_set_bits(SetBitVisitor set_bit_visitor)
            -> ystdlib::error_handling::Result<void> {
        for (size_t component_idx{}; component_idx < m_bitmap_array_length; ++component_idx) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (bitmap_component_t{} == m_bitmap_array[component_idx]) {
                continue;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto const current_component{m_bitmap_array[component_idx]};
            bitmap_component_t new_component{};
            auto const last_component_idx{m_bitmap_array_length - 1};
            auto const max_bit{
                    component_idx == last_component_idx
                            ? m_num_bits - (cNumBitsPerComponent * last_component_idx)
                            : cNumBitsPerComponent
            };
            for (size_t bit_idx{}; bit_idx < max_bit; ++bit_idx) {
                auto const current_bit{static_cast<bitmap_component_t>(1) << bit_idx};
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
    BitmapView(bitmap_component_t* bitmap_array, size_t num_bits)
            : m_bitmap_array(bitmap_array),
              m_num_bits{num_bits},
              m_bitmap_array_length{
                      (num_bits / cNumBitsPerComponent)
                      + (num_bits % cNumBitsPerComponent > 0 ? 1 : 0)
              } {}

    bitmap_component_t* m_bitmap_array{};
    size_t m_num_bits{};
    size_t m_bitmap_array_length{};
};
}  // namespace clp_s::filter

#endif

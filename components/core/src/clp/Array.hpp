#ifndef CLP_ARRAY_HPP
#define CLP_ARRAY_HPP

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Class for a run-time fix-sized array.
 * @tparam T The type of the element in the array. The type must be default constructable.
 */
template <typename T>
requires(std::is_fundamental_v<T> || std::is_default_constructible_v<T>)
class Array {
public:
    // Types
    using Iterator = T*;
    using ConstIterator = T const*;

    class OperationFailed : public TraceableException {
    public:
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException{error_code, filename, line_number},
                  m_message{std::move(message)} {}

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    // Constructors
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    explicit Array(size_t size) : m_data{std::make_unique<T[]>(size)}, m_size{size} {
        if constexpr (std::is_fundamental_v<T>) {
            memset(m_data.get(), 0, m_size * sizeof(T));
        }
    }

    // Disable copy constructor and assignment operator
    Array(Array const&) = delete;
    auto operator=(Array const&) -> Array& = delete;

    // Default move constructor and assignment operator
    Array(Array&&) = default;
    auto operator=(Array&&) -> Array& = default;

    // Destructor
    ~Array() = default;

    // Methods
    /**
     * @return Whether the array is empty.
     */
    [[nodiscard]] auto empty() const -> bool { return 0 == size(); }

    /**
     * @return The size of the array.
     */
    [[nodiscard]] auto size() const -> size_t { return m_size; }

    /**
     * @return The ptr of the underlying data buffer.
     */
    [[nodiscard]] auto data() -> T* { return m_data.get(); }

    /**
     * @return The ptr of the underlying data buffer.
     */
    [[nodiscard]] auto data() const -> T const* { return m_data.get(); }

    /**
     * @param idx
     * @return The element at the given index.
     * @throw `OperationFailed` if the given index is out of bound.
     */
    [[nodiscard]] auto at(size_t idx) -> T& {
        assert_idx(idx);
        return m_data[idx];
    }

    /**
     * @param idx
     * @return The element at the given index.
     * @throw `OperationFailed` if the given index is out of bound.
     */
    [[nodiscard]] auto at(size_t idx) const -> T const& {
        assert_idx(idx);
        return m_data[idx];
    }

    [[nodiscard]] auto begin() -> Iterator { return m_data.get(); }

    [[nodiscard]] auto end() -> Iterator { return m_data.get() + m_size; }

    [[nodiscard]] auto begin() const -> ConstIterator { return m_data.get(); }

    [[nodiscard]] auto end() const -> ConstIterator { return m_data.get() + m_size; }

private:
    /**
     * @param idx
     * @throw `OperationFailed` if the given index is out of bound.
     */
    auto assert_idx(size_t idx) -> void {
        if (idx >= m_size) {
            throw OperationFailed(
                    ErrorCode_OutOfBounds,
                    __FILE__,
                    __LINE__,
                    "`clp::Array access out of bound.`"
            );
        }
    }

    // Variables
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    std::unique_ptr<T[]> m_data;
    size_t m_size;
};
}  // namespace clp

#endif  // CLP_ARRAY_HPP

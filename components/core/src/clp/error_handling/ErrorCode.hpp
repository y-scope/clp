#ifndef CLP_ERROR_HANDLING_ERRORCODE_HPP
#define CLP_ERROR_HANDLING_ERRORCODE_HPP

#include <concepts>
#include <string>
#include <system_error>
#include <type_traits>

namespace clp::error_handling {
/**
 * Concept that defines a template parameter of an integer-based error code enumeration.
 * @tparam Type
 */
template <typename Type>
concept ErrorCodeEnumType = std::is_enum_v<Type> && requires(Type type) {
    {
        static_cast<std::underlying_type_t<Type>>(type)
    } -> std::convertible_to<int>;
};

/**
 * Template that defines a `std::error_category` of the given set of error code enumeration.
 * @tparam ErrorCodeEnum
 */
template <ErrorCodeEnumType ErrorCodeEnum>
class ErrorCategory : public std::error_category {
public:
    // Methods implementing `std::error_category`
    /**
     * Gets the error category name.
     * Note: A specialization must be explicitly implemented for each valid `ErrorCodeEnum`.
     * @return The name of the error category.
     */
    [[nodiscard]] auto name() const noexcept -> char const* override;

    /**
     * Gets the descriptive message associated with the given error.
     * @param error_num
     * @return The descriptive message for the error.
     */
    [[nodiscard]] auto message(int error_num) const -> std::string override {
        return message(static_cast<ErrorCodeEnum>(error_num));
    }

    /**
     * @param error_num
     * @param condition
     * @return Whether the error condition of the given error matches the given condition.
     */
    [[nodiscard]] auto
    equivalent(int error_num, std::error_condition const& condition) const noexcept
            -> bool override {
        return equivalent(static_cast<ErrorCodeEnum>(error_num), condition);
    }

    // Methods
    /**
     * Gets the descriptive message associated with the given error.
     * Note: A specialization must be explicitly implemented for each valid `ErrorCodeEnum`.
     * @param error_enum.
     * @return The descriptive message for the error.
     */
    [[nodiscard]] auto message(ErrorCodeEnum error_enum) const -> std::string;

    /**
     * Note: A specialization can be implemented to create error enum to error condition mappings.
     * @param error_num
     * @param condition
     * @return Whether the error condition of the given error matches the given condition.
     */
    [[nodiscard]] auto
    equivalent(ErrorCodeEnum error_enum, std::error_condition const& condition) const noexcept
            -> bool;
};

/**
 * Template class that defines an error code. An error code is represented by a error enum value and
 * the associated error category. This template class is designed to be `std::error_code`
 * compatible, meaning that every instance of this class can be used to construct a corresponded
 * `std::error_code` instance, or compare with a `std::error_code` instance to inspect a specific
 * error.
 * @tparam ErrorCodeEnum
 */
template <ErrorCodeEnumType ErrorCodeEnum>
class ErrorCode {
public:
    // Constructor
    ErrorCode(ErrorCodeEnum error) : m_error{error} {}

    /**
     * @return The underlying error code enum.
     */
    [[nodiscard]] auto get_error() const -> ErrorCodeEnum { return m_error; }

    /**
     * @return The error code as an error number.
     */
    [[nodiscard]] auto get_error_num() const -> int { return static_cast<int>(m_error); }

    /**
     * @return The reference to the singleton of the corresponded error category.
     */
    [[nodiscard]] constexpr static auto get_category() -> ErrorCategory<ErrorCodeEnum> const& {
        return cCategory;
    }

private:
    static inline ErrorCategory<ErrorCodeEnum> const cCategory;

    ErrorCodeEnum m_error;
};

/**
 * @tparam ErrorCodeEnum
 * @param error
 * @return Constructed `std::error_code` from the given `ErrorCode` instance.
 */
template <typename ErrorCodeEnum>
[[nodiscard]] auto make_error_code(ErrorCode<ErrorCodeEnum> error) -> std::error_code;

template <ErrorCodeEnumType ErrorCodeEnum>
auto ErrorCategory<ErrorCodeEnum>::equivalent(
        ErrorCodeEnum error_enum,
        std::error_condition const& condition
) const noexcept -> bool {
    return std::error_category::default_error_condition(static_cast<int>(error_enum)) == condition;
}

template <typename ErrorCodeEnum>
auto make_error_code(ErrorCode<ErrorCodeEnum> error) -> std::error_code {
    return {error.get_error_num(), ErrorCode<ErrorCodeEnum>::get_category()};
}
}  // namespace clp::error_handling

/**
 * The macro to create a specialization of `std::is_error_code_enum` for a given type T. Only types
 * that are marked with this macro will be considered as a valid CLP error code enum, and thus used
 * to specialize `ErrorCode` and `ErrorCategory` templates.
 */
// NOLINTBEGIN(bugprone-macro-parentheses, cppcoreguidelines-macro-usage)
#define CLP_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(T) \
    template <> \
    struct std::is_error_code_enum<clp::error_handling::ErrorCode<T>> : std::true_type { \
        static_assert(std::is_enum_v<T>); \
    };
// NOLINTEND(bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

#endif  // CLP_ERROR_HANDLING_ERRORCODE_HPP

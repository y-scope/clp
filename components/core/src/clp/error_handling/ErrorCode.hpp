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
    [[nodiscard]] auto message(int error_num) const -> std::string override;

    // Methods
    /**
     * Gets the descriptive message associated with the given error.
     * Note: A specialization must be explicitly implemented for each valid `ErrorCodeEnum`.
     * @param error_enum.
     * @return The descriptive message for the error.
     */
    [[nodiscard]] auto message(ErrorCodeEnum error_enum) const -> std::string;
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
     * @return The error code as an error number.
     */
    [[nodiscard]] auto get_errno() const -> int;

    /**
     * @return The underlying error code enum.
     */
    [[nodiscard]] auto get_err_enum() const -> ErrorCodeEnum;

    /**
     * @return The reference to the singleton of the corresponded error category.
     */
    [[nodiscard]] static auto get_category() -> ErrorCategory<ErrorCodeEnum> const&;

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
auto ErrorCategory<ErrorCodeEnum>::message(int error_num) const -> std::string {
    return message(static_cast<ErrorCodeEnum>(error_num));
}

template <ErrorCodeEnumType ErrorCodeEnum>
auto ErrorCode<ErrorCodeEnum>::get_errno() const -> int {
    return static_cast<int>(m_error);
}

template <ErrorCodeEnumType ErrorCodeEnum>
auto ErrorCode<ErrorCodeEnum>::get_err_enum() const -> ErrorCodeEnum {
    return m_error;
}

template <ErrorCodeEnumType ErrorCodeEnum>
auto ErrorCode<ErrorCodeEnum>::get_category() -> ErrorCategory<ErrorCodeEnum> const& {
    return ErrorCode<ErrorCodeEnum>::cCategory;
}

template <typename ErrorCodeEnum>
[[nodiscard]] auto make_error_code(ErrorCode<ErrorCodeEnum> error) -> std::error_code {
    return {error.get_errno(), ErrorCode<ErrorCodeEnum>::get_category()};
}
}  // namespace clp::error_handling

#endif  // CLP_ERROR_HANDLING_ERRORCODE_HPP

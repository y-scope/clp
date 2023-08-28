#ifndef SPDLOG_WITH_SPECIALIZATIONS_HPP
#define SPDLOG_WITH_SPECIALIZATIONS_HPP

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "ErrorCode.hpp"
#include "ffi/search/ExactVariableToken.hpp"
#include "ffi/search/WildcardToken.hpp"

template <>
struct fmt::formatter<ErrorCode> {
    template <typename ParseContext>
    constexpr auto parse (ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format (
            const ErrorCode& error_code,
            FormatContext& ctx
    ) {
        return fmt::format_to(ctx.out(), "{}", static_cast<size_t>(error_code));
    }
};

template <typename encoded_variable_t>
struct fmt::formatter<ffi::search::ExactVariableToken<encoded_variable_t>> {
    template <typename ParseContext>
    constexpr auto parse (ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format (
            const ffi::search::ExactVariableToken<encoded_variable_t>& v,
            FormatContext& ctx
    ) {
        return fmt::format_to(ctx.out(), "ExactVariableToken(\"{}\") as {}", v.get_value(),
                              v.get_encoded_value());
    }
};

template<typename encoded_variable_t>
struct fmt::formatter<ffi::search::WildcardToken<encoded_variable_t>> {
    template <typename ParseContext>
    constexpr auto parse (ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format (
            const ffi::search::WildcardToken<encoded_variable_t>& v,
            FormatContext& ctx
    ) {
        return fmt::format_to(ctx.out(), "WildcardToken(\"{}\") as {}TokenType({}){}",
                              v.get_value(),
                              v.has_prefix_star_wildcard() ? "*" : "",
                              v.get_current_interpretation(),
                              v.has_suffix_star_wildcard() ? "*" : "");
    }
};

#endif // SPDLOG_WITH_SPECIALIZATIONS_HPP

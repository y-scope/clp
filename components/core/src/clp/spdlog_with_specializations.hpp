#ifndef CLP_SPDLOG_WITH_SPECIALIZATIONS_HPP
#define CLP_SPDLOG_WITH_SPECIALIZATIONS_HPP

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "ErrorCode.hpp"
#include "ffi/search/ExactVariableToken.hpp"
#include "ffi/search/WildcardToken.hpp"

template <>
struct fmt::formatter<clp::ErrorCode> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(clp::ErrorCode const& error_code, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", static_cast<size_t>(error_code));
    }
};

template <typename encoded_variable_t>
struct fmt::formatter<clp::ffi::search::ExactVariableToken<encoded_variable_t>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto
    format(clp::ffi::search::ExactVariableToken<encoded_variable_t> const& v, FormatContext& ctx) {
        return fmt::format_to(
                ctx.out(),
                "ExactVariableToken(\"{}\") as {}",
                v.get_value(),
                v.get_encoded_value()
        );
    }
};

template <typename encoded_variable_t>
struct fmt::formatter<clp::ffi::search::WildcardToken<encoded_variable_t>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(clp::ffi::search::WildcardToken<encoded_variable_t> const& v, FormatContext& ctx) {
        return fmt::format_to(
                ctx.out(),
                "WildcardToken(\"{}\") as {}TokenType({}){}",
                v.get_value(),
                v.has_prefix_star_wildcard() ? "*" : "",
                v.get_current_interpretation(),
                v.has_suffix_star_wildcard() ? "*" : ""
        );
    }
};

#endif  // CLP_SPDLOG_WITH_SPECIALIZATIONS_HPP

#ifndef FFI_SEARCH_FMTLIB_SPECIALIZATIONS_HPP
#define FFI_SEARCH_FMTLIB_SPECIALIZATIONS_HPP

// fmtlib
#include <fmt/format.h>

// Project headers
#include "ExactVariableToken.hpp"
#include "WildcardToken.hpp"

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

#endif // FFI_SEARCH_FMTLIB_SPECIALIZATIONS_HPP

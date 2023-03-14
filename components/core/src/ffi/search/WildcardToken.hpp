#ifndef FFI_WILDCARDTOKEN_HPP
#define FFI_WILDCARDTOKEN_HPP

// C++ standard libraries
#include <vector>

// fmtlib
#include <fmt/core.h>
#include <fmt/format.h>

// Project headers
#include "../../TraceableException.hpp"
#include "QueryToken.hpp"

namespace ffi::search {
    /**
     * A token containing one or more wildcards. Note that the original query
     * string is stored by reference, so it must remain valid while the token
     * exists.
     * @tparam encoded_variable_t
     */
    template <typename encoded_variable_t>
    class WildcardToken : public QueryToken {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            [[nodiscard]] const char* what () const noexcept override {
                return "ffi::search::WildcardToken operation failed";
            }
        };

        // Constructors
        WildcardToken (std::string_view query, size_t begin_pos, size_t end_pos);

        // Methods
        bool operator== (const WildcardToken& rhs) const {
            return static_cast<const ffi::search::QueryToken&>(*this)
                   == static_cast<const ffi::search::QueryToken&>(rhs)
                   && m_has_prefix_star_wildcard == rhs.m_has_prefix_star_wildcard
                   && m_has_suffix_star_wildcard == rhs.m_has_suffix_star_wildcard
                   && m_possible_variable_types == rhs.m_possible_variable_types
                   && m_current_interpretation_idx == rhs.m_current_interpretation_idx;
        }
        bool operator!= (const WildcardToken& rhs) const {
            return !(rhs == *this);
        }

        /**
         * Adds this token to the given logtype query
         * @param logtype_query
         * @return true if the token is interpreted as a variable
         * @return false if the token is interpreted as static text
         */
        bool add_to_logtype_query (std::string& logtype_query) const;

        /**
         * Advances to the next interpretation of this WildcardToken
         * @return true if there was another interpretation to advance to
         * @return false if we overflowed to the first interpretation
         */
        bool next_interpretation ();

        [[nodiscard]] bool has_suffix_star_wildcard () const {
            return m_has_suffix_star_wildcard;
        }

        [[nodiscard]] bool has_prefix_star_wildcard () const {
            return m_has_prefix_star_wildcard;
        }

        [[nodiscard]] TokenType get_current_interpretation () const {
            return m_possible_variable_types[m_current_interpretation_idx];
        }

    private:
        bool m_has_prefix_star_wildcard;
        bool m_has_suffix_star_wildcard;
        std::vector<TokenType> m_possible_variable_types;
        size_t m_current_interpretation_idx;
    };
}

// Template specialization for SPDLOG
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

#endif // FFI_WILDCARDTOKEN_HPP

#ifndef FFI_SEARCH_EXACTVARIABLETOKEN_HPP
#define FFI_SEARCH_EXACTVARIABLETOKEN_HPP

// Project headers
#include "../../Defs.h"
#include "../encoding_methods.hpp"
#include "QueryToken.hpp"

namespace ffi::search {
    /**
     * A token representing an exact variable (as opposed to a variable with
     * wildcards). Note that the original query string is stored by reference,
     * so it must remain valid while the token exists.
     * @tparam encoded_variable_t Type for encoded variable values
     */
    template<typename encoded_variable_t>
    class ExactVariableToken : public QueryToken {
    public:
        // Constructors
        /**
         * Constructs an exact variable token. NOTE: It's the callers
         * responsibility to ensure that the token is indeed a variable.
         * @param query
         * @param begin_pos
         * @param end_pos
         */
        ExactVariableToken (std::string_view query, size_t begin_pos, size_t end_pos);

        // Methods
        bool operator== (const ExactVariableToken& rhs) const {
            return static_cast<const ffi::search::QueryToken&>(*this)
                   == static_cast<const ffi::search::QueryToken&>(rhs)
                   && m_encoded_value == rhs.m_encoded_value
                   && m_placeholder == rhs.m_placeholder;
        }
        bool operator!= (const ExactVariableToken& rhs) const {
            return !(rhs == *this);
        }

        void add_to_logtype_query (std::string& logtype_query) const {
            logtype_query += enum_to_underlying_type(m_placeholder);
        }

        [[nodiscard]] encoded_variable_t get_encoded_value () const { return m_encoded_value; }
        [[nodiscard]] VariablePlaceholder get_placeholder () const { return m_placeholder; }

    private:
        encoded_variable_t m_encoded_value;
        VariablePlaceholder m_placeholder;
    };
}

#endif // FFI_SEARCH_EXACTVARIABLETOKEN_HPP

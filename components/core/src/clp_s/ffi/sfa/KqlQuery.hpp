#ifndef CLP_S_FFI_SFA_KQLQUERY_HPP
#define CLP_S_FFI_SFA_KQLQUERY_HPP

#include <memory>
#include <string>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

namespace clp_s::search { namespace ast {
class Expression;
}}  // namespace clp_s::search::ast

namespace clp_s::ffi::sfa {
class KqlQuery {
public:
    /**
     * Parses a KQL expression for later archive filtering.
     *
     * @param query_str KQL query string.
     * @param ignore_case Whether string matching should ignore case.
     * @return A result containing the parsed `KqlQuery` on success, or an error code indicating
     * the failure:
     * - `SfaErrorCodeEnum::Failure` if parsing fails or the query is logically false.
     * - `SfaErrorCodeEnum::NoMemory` if allocating query state fails.
     */
    [[nodiscard]] static auto create(std::string_view query_str, bool ignore_case = false)
            -> ystdlib::error_handling::Result<KqlQuery>;

    /**
     * @return A deep copy of the parsed KQL expression tree.
     */
    [[nodiscard]] auto copy_expression() const -> std::shared_ptr<clp_s::search::ast::Expression>;

    /**
     * @return Whether string matching should ignore case.
     */
    [[nodiscard]] auto ignore_case() const -> bool { return m_ignore_case; }

private:
    explicit KqlQuery(std::shared_ptr<clp_s::search::ast::Expression> expression, bool ignore_case)
            : m_expression{std::move(expression)},
              m_ignore_case{ignore_case} {}

    std::shared_ptr<clp_s::search::ast::Expression> m_expression;
    bool m_ignore_case{false};
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_KQLQUERY_HPP

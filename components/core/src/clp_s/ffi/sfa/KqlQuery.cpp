#include "KqlQuery.hpp"

#include <exception>
#include <memory>
#include <new>
#include <sstream>
#include <string_view>

#include <spdlog/spdlog.h>

#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>
#include <clp_s/search/kql/kql.hpp>

#include "SfaErrorCode.hpp"

namespace clp_s::ffi::sfa {
auto KqlQuery::create(std::string_view query_str, bool ignore_case)
        -> ystdlib::error_handling::Result<KqlQuery> {
    try {
        auto query_stream = std::istringstream{std::string{query_str}};
        auto expr = clp_s::search::kql::parse_kql_expression(query_stream);
        if (nullptr == expr) {
            SPDLOG_ERROR("Failed to parse KQL query: '{}'.", query_str);
            return SfaErrorCode{SfaErrorCodeEnum::Failure};
        }

        expr = clp_s::search::ast::preprocess_query(expr);
        if (std::dynamic_pointer_cast<clp_s::search::ast::EmptyExpr>(expr)) {
            SPDLOG_ERROR("KQL query '{}' is logically false.", query_str);
            return SfaErrorCode{SfaErrorCodeEnum::Failure};
        }

        return KqlQuery{std::move(expr), ignore_case};
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create KqlQuery: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating KqlQuery: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}

auto KqlQuery::copy_expression() const -> std::shared_ptr<clp_s::search::ast::Expression> {
    return m_expression->copy();
}
}  // namespace clp_s::ffi::sfa

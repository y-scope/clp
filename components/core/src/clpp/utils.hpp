#ifndef CLPP_UTILS_HPP
#define CLPP_UTILS_HPP

#include <memory>
#include <string_view>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

namespace clpp {
/**
 * Builds a log-surgeon parser from a parsing specification text, registering the encoding patterns
 * from `clpp::cEncodingPatterns`.
 *
 * @param spec_str The parsing specification text.
 * @return A result containing the built parser, or an error code indicating the failure:
 * - clpp::ClppErrorCodeEnum::BadParam if `spec_str` is empty or an encoding fails to be added.
 */
[[nodiscard]] auto build_parsing_spec(std::string_view spec_str)
        -> ystdlib::error_handling::Result<std::unique_ptr<log_surgeon::Parser>>;

/**
 * Collects the chain of ancestor matches that become `ParentRule` schema-tree nodes for a leaf,
 * in root->leaf order.
 *
 * If the leaf is a root rule `chain` is left empty. Otherwise every ancestor from the leaf's parent
 * up to and including the root-rule ancestor becomes an element.
 *
 * @param leaf The leaf match whose ancestor chain to collect.
 * @param chain Re-used output vector that is cleared and filled each call.
 */
auto
collect_parent_chain(log_surgeon::Match const& leaf, std::vector<log_surgeon::Match const*>& chain)
        -> void;
}  // namespace clpp

#endif  // CLPP_UTILS_HPP

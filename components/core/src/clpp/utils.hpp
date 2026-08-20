#ifndef CLPP_UTILS_HPP
#define CLPP_UTILS_HPP

#include <string>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ReaderInterface.hpp>

namespace clpp {
/**
 * Builds a log-surgeon parser from a parsing specification read with a `clp::ReaderInterface`,
 * and registers the encoding patterns from `clpp::cEncodingPatterns`.
 *
 * @param reader A `clp::ReaderInterface` positioned at the beginning of the parsing specification.
 * @return A result containing a pair of:
 * - The built parser.
 * - The parsing spec contents/text.
 * or an error code indicating the failure:
 * - clpp::ClppErrorCodeEnum::BadParam if reading the spec fails, it is empty, or adding an encoding
 * pattern fails.
 */
[[nodiscard]] auto build_parser(clp::ReaderInterface& reader)
        -> ystdlib::error_handling::Result<std::pair<log_surgeon::Parser, std::string>>;

/**
 * Collects the chain of ancestor matches that become `ParentRule` schema-tree nodes for a leaf,
 * in leaf->root order.
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

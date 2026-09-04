#ifndef CLPP_UTILS_HPP
#define CLPP_UTILS_HPP

#include <string>
#include <utility>

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
}  // namespace clpp

#endif  // CLPP_UTILS_HPP

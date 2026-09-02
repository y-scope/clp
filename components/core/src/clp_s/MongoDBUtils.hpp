#ifndef CLP_S_MONGODBUTILS_HPP
#define CLP_S_MONGODBUTILS_HPP

#include <mongocxx/exception/bulk_write_exception.hpp>

namespace clp_s {
/**
 * Returns whether the bulk write failed only because some documents already exist.
 *
 * Command and write-concern errors are rejected since they mean MongoDB did not confirm the
 * outcome of the entire batch. At least one write error must be present, and every write error
 * must be a duplicate-key error.
 * @param exception The exception containing the raw MongoDB bulk-write reply.
 * @return true if the reply contains only duplicate-key write errors, false otherwise.
 */
[[nodiscard]] auto contains_only_duplicate_key_write_errors(
        mongocxx::bulk_write_exception const& exception
) -> bool;
}  // namespace clp_s

#endif  // CLP_S_MONGODBUTILS_HPP

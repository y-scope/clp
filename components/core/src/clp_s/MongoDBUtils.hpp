#ifndef CLP_S_MONGODBUTILS_HPP
#define CLP_S_MONGODBUTILS_HPP

#include <cstddef>

#include <mongocxx/exception/bulk_write_exception.hpp>

namespace clp_s {
/**
 * Returns whether the bulk write failed only because some documents already exist.
 *
 * Command and write-concern errors are rejected since they mean MongoDB did not confirm the outcome
 * of the entire batch. The number of inserted documents and write errors must account for every
 * submitted document, and every write error must be a duplicate-key error.
 * @param exception The exception containing the raw MongoDB bulk-write reply.
 * @param num_documents The number of documents submitted in the bulk write.
 * @return Whether the reply accounts for every document using successful inserts and duplicate-key
 * errors only.
 */
[[nodiscard]] auto contains_only_duplicate_key_write_errors(
        mongocxx::bulk_write_exception const& exception,
        size_t num_documents
) -> bool;
}  // namespace clp_s

#endif  // CLP_S_MONGODBUTILS_HPP

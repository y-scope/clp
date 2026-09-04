#include "MongoDBUtils.hpp"

#include <cstdint>

#include <bsoncxx/types.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>

namespace clp_s {
namespace {
constexpr int32_t cDuplicateKeyErrorCode{11'000};

/**
 * Checks whether an aggregated bulk-write reply contains any command errors.
 * @param reply The raw MongoDB bulk-write reply.
 * @return Whether the reply contains a command error.
 */
[[nodiscard]] auto has_command_errors(bsoncxx::document::view const& reply) -> bool;

/**
 * Checks whether a bulk-write reply contains any write-concern errors.
 * @param reply The raw MongoDB bulk-write reply.
 * @return Whether the reply contains a write-concern error.
 */
[[nodiscard]] auto has_write_concern_errors(bsoncxx::document::view const& reply) -> bool;

/**
 * Checks whether an entry from a bulk-write reply's `writeErrors` array is a duplicate-key error.
 * @param write_error The write-error entry to inspect.
 * @return Whether the entry has MongoDB's duplicate-key error code.
 */
[[nodiscard]] auto is_duplicate_key_write_error(bsoncxx::array::element const& write_error) -> bool;

[[nodiscard]] auto has_command_errors(bsoncxx::document::view const& reply) -> bool {
    auto const errors_element = reply["errorReplies"];
    if (false == static_cast<bool>(errors_element)) {
        return false;
    }
    if (bsoncxx::type::k_array != errors_element.type()) {
        return true;
    }
    auto const errors = errors_element.get_array().value;
    return false == errors.empty();
}

[[nodiscard]] auto has_write_concern_errors(bsoncxx::document::view const& reply) -> bool {
    if (static_cast<bool>(reply["writeConcernError"])) {
        return true;
    }

    auto const errors_element = reply["writeConcernErrors"];
    if (false == static_cast<bool>(errors_element)) {
        return false;
    }
    if (bsoncxx::type::k_array != errors_element.type()) {
        return true;
    }
    auto const errors = errors_element.get_array().value;
    return false == errors.empty();
}

[[nodiscard]] auto is_duplicate_key_write_error(bsoncxx::array::element const& write_error)
        -> bool {
    if (bsoncxx::type::k_document != write_error.type()) {
        return false;
    }

    auto const code = write_error.get_document().value["code"];
    if (false == static_cast<bool>(code)) {
        return false;
    }
    if (bsoncxx::type::k_int32 == code.type()) {
        return cDuplicateKeyErrorCode == code.get_int32().value;
    }
    if (bsoncxx::type::k_int64 == code.type()) {
        return cDuplicateKeyErrorCode == code.get_int64().value;
    }
    return false;
}
}  // namespace

auto contains_only_duplicate_key_write_errors(
        mongocxx::bulk_write_exception const& exception,
        size_t num_documents
) -> bool {
    auto const& raw_server_error = exception.raw_server_error();
    if (false == raw_server_error.has_value()) {
        return false;
    }

    auto const reply = raw_server_error->view();
    if (has_command_errors(reply) || has_write_concern_errors(reply)) {
        return false;
    }

    auto const write_errors_element = reply["writeErrors"];
    if (false == static_cast<bool>(write_errors_element)
        || bsoncxx::type::k_array != write_errors_element.type())
    {
        return false;
    }

    auto const write_errors = write_errors_element.get_array().value;
    if (write_errors.empty()) {
        return false;
    }

    size_t num_write_errors{0};
    for (auto const& write_error : write_errors) {
        if (false == is_duplicate_key_write_error(write_error)) {
            return false;
        }
        ++num_write_errors;
    }

    auto const num_inserted_element = reply["nInserted"];
    if (false == static_cast<bool>(num_inserted_element)
        || bsoncxx::type::k_int32 != num_inserted_element.type())
    {
        return false;
    }
    auto const num_inserted = num_inserted_element.get_int32().value;
    if (num_inserted < 0) {
        return false;
    }

    return static_cast<size_t>(num_inserted) + num_write_errors == num_documents;
}
}  // namespace clp_s

#include "MongoDBUtils.hpp"

#include <algorithm>

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

auto contains_only_duplicate_key_write_errors(mongocxx::bulk_write_exception const& exception)
        -> bool {
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
    return std::all_of(write_errors.begin(), write_errors.end(), is_duplicate_key_write_error);
}
}  // namespace clp_s

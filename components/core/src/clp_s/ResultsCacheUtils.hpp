#ifndef CLP_S_RESULTSCACHEUTILS_HPP
#define CLP_S_RESULTSCACHEUTILS_HPP

#include <string_view>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/TraceableException.hpp>

namespace clp_s {
class ResultsCacheConnectionError : public TraceableException {
public:
    ResultsCacheConnectionError(ErrorCode error_code, char const* const filename, int line_number)
            : TraceableException{error_code, filename, line_number} {}
};

/**
 * Connects to the results cache and returns the requested collection.
 * @param uri
 * @param collection
 * @param client Returns the connected client.
 * @return The collection.
 * @throw ResultsCacheConnectionError if connecting to the results cache fails.
 */
[[nodiscard]] auto connect_to_results_cache(
        std::string_view uri,
        std::string_view collection,
        mongocxx::client& client
) -> mongocxx::collection;
}  // namespace clp_s

#endif  // CLP_S_RESULTSCACHEUTILS_HPP

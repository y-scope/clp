#ifndef CLP_S_RESULTSCACHEUTILS_HPP
#define CLP_S_RESULTSCACHEUTILS_HPP

#include <string>
#include <string_view>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include "ErrorCode.hpp"

namespace clp_s {
/**
 * Connects to the results cache and returns the requested collection.
 * @tparam OperationFailedT The type of exception to throw on failure.
 * @param uri
 * @param collection
 * @param client Returns the connected client.
 * @return The collection.
 */
template <typename OperationFailedT>
auto connect_to_results_cache(
        std::string_view uri,
        std::string_view collection,
        mongocxx::client& client
) -> mongocxx::collection {
    try {
        auto mongo_uri = mongocxx::uri{std::string{uri}};
        client = mongocxx::client(mongo_uri);
        return client[mongo_uri.database()][std::string{collection}];
    } catch (mongocxx::exception const& e) {
        throw OperationFailedT(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s

#endif  // CLP_S_RESULTSCACHEUTILS_HPP

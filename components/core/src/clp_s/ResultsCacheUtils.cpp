#include "ResultsCacheUtils.hpp"

#include <string>
#include <string_view>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include <clp_s/ErrorCode.hpp>

namespace clp_s {
auto connect_to_results_cache(
        std::string_view uri,
        std::string_view collection,
        mongocxx::client& client
) -> mongocxx::collection {
    try {
        auto mongo_uri = mongocxx::uri{std::string{uri}};
        client = mongocxx::client(mongo_uri);
        return client[mongo_uri.database()][std::string{collection}];
    } catch (mongocxx::exception const&) {
        throw ResultsCacheConnectionError(
                ErrorCode::ErrorCodeBadParamDbUri,
                __FILENAME__,
                __LINE__
        );
    }
}
}  // namespace clp_s

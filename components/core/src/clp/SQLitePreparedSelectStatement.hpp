#ifndef CLP_SQLITEPREPAREDSELECTSTATEMENT_HPP
#define CLP_SQLITEPREPAREDSELECTSTATEMENT_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <optional>

#include "SQLitePreparedStatement.hpp"

/**
 * This class provides abstractions to a sqlite select statement. It maintains a map to lookup the
 * index of a selected column in the statement by giving its name.
*/
namespace clp {
class SQLitePreparedSelectStatement : public SQLitePreparedStatement {
public:
    SQLitePreparedSelectStatement(
        std::vector<std::string> const& columns_to_select,
        std::string_view table_name,
        std::vector<std::string> where_clause,
        std::vector<std::string> ordering_clause
    );
private:
    
};
}

#endif

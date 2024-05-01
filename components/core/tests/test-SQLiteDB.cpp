#include <filesystem>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/SQLiteDB.hpp"
#include "../src/clp/SQLitePreparedStatement.hpp"
#include "../src/clp/SQLitePreparedSelectStatement.hpp"
#include "../src/clp/streaming_archive/Constants.hpp"

namespace {
[[nodiscard]] auto get_test_sqlite_db_abs_path() -> std::filesystem::path {
    std::filesystem::path const file_path{__FILE__};
    auto const test_root_path{file_path.parent_path()};
    return test_root_path / "test_sqlite_db/test.db";
}
}

TEST_CASE("sqlite_db_prepared_select_statement", "[SQliteDB]") {
    clp::SQLiteDB sqlite_db;
    sqlite_db.open(get_test_sqlite_db_abs_path().string());
}
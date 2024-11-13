#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/hash_utils.hpp"
#include "../src/clp/type_utils.hpp"

using clp::convert_to_hex_string;
using clp::ErrorCode_Success;
using clp::get_hmac_sha256_hash;
using clp::get_sha256_hash;
using clp::size_checked_pointer_cast;
using std::string_view;
using std::vector;

TEST_CASE("test_sha256", "[hash_utils]") {
    constexpr string_view cInputString{"ThisIsARandomTestInput"};
    constexpr string_view cReferenceSha256{
            "c3a1d9f04ada1198c4c63bf51d9933fc2cc216429275cadabdcb2178775add0c"
    };
    vector<unsigned char> hash;

    REQUIRE(ErrorCode_Success
            == get_sha256_hash(
                    {size_checked_pointer_cast<unsigned char const>(cInputString.data()),
                     cInputString.size()},
                    hash
            ));
    REQUIRE(convert_to_hex_string(hash) == cReferenceSha256);
}

TEST_CASE("test_hmac", "[hash_utils]") {
    constexpr string_view cInputString{"ThisIsARandomTestInput"};
    constexpr string_view cInputKey{"ThisIsATestKey"};
    constexpr string_view cReferenceHmacSha256{
            "38373057694c1038a6895212bea46849eb7a59b73a2ec175883ae095fb91ffda"
    };
    vector<unsigned char> hmac_hash;

    REQUIRE(ErrorCode_Success
            == get_hmac_sha256_hash(
                    {size_checked_pointer_cast<unsigned char const>(cInputString.data()),
                     cInputString.size()},
                    {size_checked_pointer_cast<unsigned char const>(cInputKey.data()),
                     cInputKey.size()},
                    hmac_hash
            ));
    REQUIRE(convert_to_hex_string(hmac_hash) == cReferenceHmacSha256);
}

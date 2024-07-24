#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/hash_utils.hpp"
#include "../src/clp/type_utils.hpp"
using clp::convert_hash_to_hex_string;
using clp::ErrorCode_Success;
using clp::get_hmac_sha256_hash;
using clp::get_sha256_hash;
using clp::size_checked_pointer_cast;
using std::string_view;
using std::vector;

TEST_CASE("test_sha256", "[hash_utils]") {
    constexpr string_view input_string{"ThisIsARandomTestInput"};
    constexpr string_view reference_sha256{
            "c3a1d9f04ada1198c4c63bf51d9933fc2cc216429275cadabdcb2178775add0c"
    };
    vector<unsigned char> hash{};

    CHECK(ErrorCode_Success
          == get_sha256_hash(
                  {size_checked_pointer_cast<unsigned char const>(input_string.data()),
                   input_string.size()},
                  hash
          ));
    CHECK(convert_hash_to_hex_string(hash) == reference_sha256);
}

TEST_CASE("test_hmac", "[hash_utils]") {
    string_view input_string{"ThisIsARandomTestInput"};
    string_view input_key{"ThisIsATestKey"};
    string_view reference_hmac_sha256{
            "38373057694c1038a6895212bea46849eb7a59b73a2ec175883ae095fb91ffda"
    };
    vector<unsigned char> hmac_hash{};

    CHECK(ErrorCode_Success
          == get_hmac_sha256_hash(
                  {size_checked_pointer_cast<unsigned char const>(input_string.data()),
                   input_string.size()},
                  {size_checked_pointer_cast<unsigned char const>(input_key.data()),
                   input_key.size()},
                  hmac_hash
          ));
    CHECK(convert_hash_to_hex_string(hmac_hash) == reference_hmac_sha256);
}

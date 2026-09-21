#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <clpp/TextShape.hpp>

namespace clpp::test {
TEST_CASE("text_shape_count_placeholders_before", "[TextShape]") {
    constexpr std::string_view cShape{"a %%b %r.x% c%%d %r.y%%r.z% e"};
    TextShape<std::string_view> const shape{cShape};
    REQUIRE(0 == shape.count_placeholders_before(0));
    REQUIRE(0 == shape.count_placeholders_before(cShape.find("%r.x%")));
    REQUIRE(1 == shape.count_placeholders_before(cShape.find("%r.x%") + 1));
    REQUIRE(1 == shape.count_placeholders_before(cShape.find("%r.y%")));
    REQUIRE(2 == shape.count_placeholders_before(cShape.find("%r.z%")));
    REQUIRE(3 == shape.count_placeholders_before(cShape.size()));
}
}  // namespace clpp::test

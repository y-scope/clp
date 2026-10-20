#include <cstddef>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <clpp/Defs.hpp>
#include <clpp/ParentRuleShapes.hpp>
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

TEST_CASE("text_shape_narrow_to_parent_rule", "[TextShape]") {
    // Three `g` occurrences: the first has no leaf placeholders, so it shares leaf position 0 with
    // the second and is only distinguishable by placeholder count.
    constexpr std::string_view cShape{"[x] [x%g.v%] [y%g.v%]"};
    TextShape<std::string_view> const shape{cShape};
    auto const make_g_shape{[](size_t start, size_t size) -> ParentRuleShape {
        return {"g", static_cast<shape_size_t>(start), static_cast<shape_size_t>(size)};
    }};
    ParentRuleShapes const parent_rule_shapes{
            make_g_shape(cShape.find("[x]"), 3),
            make_g_shape(cShape.find("[x%"), 8),
            make_g_shape(cShape.find("[y%"), 8)
    };

    REQUIRE("[x]" == shape.narrow_to_parent_rule(parent_rule_shapes, "g", 0, 0).view());
    REQUIRE("[x%g.v%]" == shape.narrow_to_parent_rule(parent_rule_shapes, "g", 0, 1).view());
    REQUIRE("[y%g.v%]" == shape.narrow_to_parent_rule(parent_rule_shapes, "g", 1, 1).view());
    REQUIRE(shape.narrow_to_parent_rule(parent_rule_shapes, "g", 2, 1).empty());
    REQUIRE(shape.narrow_to_parent_rule(parent_rule_shapes, "g", 0, 2).empty());
    REQUIRE(shape.narrow_to_parent_rule(parent_rule_shapes, "missing", 0, 1).empty());
}
}  // namespace clpp::test

#ifndef CLPP_PARENTRULESHAPES_HPP
#define CLPP_PARENTRULESHAPES_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Array.hpp>
#include <clpp/Defs.hpp>

namespace clpp {
/*
 * Stores the parent rule shapes for a log shape in an archive.
 */
class ParentRuleShapes {
public:
    // Types
    /**
     * A parent rule shape represented as a string view into the log shape string.
     * Its name is the corresponding name from the parsing spec.
     */
    struct ParentRuleShape {
        // Constructors
        ParentRuleShape(std::string_view name, shape_size_t start, shape_size_t size)
                : m_name(name),
                  m_start(start),
                  m_size(size) {}

        // Data members
        std::string m_name;
        shape_size_t m_start;
        shape_size_t m_size;
    };

    // Methods
    [[nodiscard]] auto compress(clp_s::ZstdCompressor& compressor) const
            -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] static auto decompress(clp_s::ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<ParentRuleShapes>;

    /**
     * Appends a parent rule shape, enforcing that the start position (within the log shape), size,
     * and name length fit in `shape_size_t`.
     *
     * @param name The qualified name of the parent rule.
     * @param start The shape's start position within the log shape.
     * @param size The shape's size.
     * @return A reference to the inserted shape.
     * @throws std::system_error (`ClppErrorCodeEnum::OutOfBounds`) if any value or the container
     * size exceeds `clpp::cMaxShapeSize`.
     */
    auto emplace_parent_rule_shape(std::string_view name, size_t start, size_t size)
            -> ParentRuleShape&;

    auto at(shape_size_t i) -> ParentRuleShape& { return m_parent_rule_shapes.at(i); }

    [[nodiscard]] auto get() const -> std::vector<ParentRuleShape> const& {
        return m_parent_rule_shapes;
    }

private:
    // Data members
    std::vector<ParentRuleShape> m_parent_rule_shapes;
};

using ParentRuleShapesArray = Array<ParentRuleShapes, clpp::log_shape_id_t>;
}  // namespace clpp

#endif  // CLPP_PARENTRULESHAPES_HPP

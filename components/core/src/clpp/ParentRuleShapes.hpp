#ifndef CLPP_PARENTRULESHAPES_HPP
#define CLPP_PARENTRULESHAPES_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Defs.hpp>

namespace clpp {
/**
 * A parent rule shape represented as a range into a log shape string. Its name is the
 * corresponding fully-qualified name from the parsing spec.
 */
struct ParentRuleShape {
    ParentRuleShape(std::string_view name, shape_size_t start, shape_size_t size)
            : m_name(name),
              m_start(start),
              m_size(size) {}

    std::string m_name;
    shape_size_t m_start;
    shape_size_t m_size;
};

/**
 * The parent rule shapes of a single log shape, in document order.
 */
using ParentRuleShapes = std::vector<ParentRuleShape>;

/**
 * The parent rule shapes of every log shape in an archive, indexed by log shape ID.
 */
class ParentRuleShapesArray {
public:
    // Methods
    /**
     * Writes all parent rule shapes to `compressor`.
     * @param compressor
     */
    auto compress(clp_s::ZstdCompressor& compressor) const -> void;

    /**
     * Reads parent rule shapes from `decompressor`, appending to any already present.
     * @param decompressor
     * @return Success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the stream ends early or is malformed.
     * @throws Forwards `add_log_shape`'s exceptions.
     */
    [[nodiscard]] auto decompress(clp_s::ZstdDecompressor& decompressor)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Appends the parent rule shapes for the next log shape ID.
     * @param shapes
     * @throws std::system_error (`ClppErrorCodeEnum::OutOfBounds`) if the number of log shapes
     * would exceed `clpp::cMaxLogShapeId`, or if any name length in `shapes`, or the number of
     * shapes, exceeds `clpp::cMaxShapeSize`.
     */
    auto add_log_shape(ParentRuleShapes shapes) -> void;

    [[nodiscard]] auto get(log_shape_id_t log_shape_id) const -> ParentRuleShapes const& {
        return m_shapes_per_log_shape.at(log_shape_id);
    }

    [[nodiscard]] auto size() const -> log_shape_id_t {
        return static_cast<log_shape_id_t>(m_shapes_per_log_shape.size());
    }

    auto clear() -> void {
        m_shapes_per_log_shape.clear();
        m_data_size = 0;
    }

    /**
     * @return The number of bytes `compress` would write for the current shapes.
     */
    [[nodiscard]] auto get_data_size() const -> size_t {
        return sizeof(log_shape_id_t) + m_data_size;
    }

private:
    // Data members
    std::vector<ParentRuleShapes> m_shapes_per_log_shape;
    size_t m_data_size{};
};
}  // namespace clpp

#endif  // CLPP_PARENTRULESHAPES_HPP

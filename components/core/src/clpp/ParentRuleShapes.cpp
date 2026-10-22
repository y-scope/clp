#include "ParentRuleShapes.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <system_error>
#include <utility>

#include <ystdlib/error_handling/ErrorCode.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
namespace {
/**
 * @param shapes
 * @return The number of bytes `ParentRuleShapesArray::compress` writes for `shapes`.
 */
[[nodiscard]] auto get_shapes_data_size(ParentRuleShapes const& shapes) -> size_t;

/**
 * @param shapes
 * @return Whether every value in `shapes`, and its length, fits in `shape_size_t`.
 */
[[nodiscard]] auto shapes_fit_in_shape_size(ParentRuleShapes const& shapes) -> bool;

auto get_shapes_data_size(ParentRuleShapes const& shapes) -> size_t {
    constexpr size_t cFixedFieldsSizePerShape{3 * sizeof(shape_size_t)};
    size_t data_size{sizeof(shape_size_t)};
    for (auto const& shape : shapes) {
        data_size += cFixedFieldsSizePerShape + shape.m_name.size();
    }
    return data_size;
}

auto shapes_fit_in_shape_size(ParentRuleShapes const& shapes) -> bool {
    return shapes.size() <= cMaxShapeSize
           && std::ranges::all_of(shapes, [](ParentRuleShape const& shape) -> bool {
                  return shape.m_name.size() <= cMaxShapeSize;
              });
}
}  // namespace

auto ParentRuleShapesArray::compress(clp_s::ZstdCompressor& compressor) const -> void {
    compressor.write_numeric_value(size());
    for (auto const& shapes : m_shapes_per_log_shape) {
        compressor.write_numeric_value(static_cast<shape_size_t>(shapes.size()));
        for (auto const& shape : shapes) {
            compressor.write_numeric_value(static_cast<shape_size_t>(shape.m_name.size()));
            compressor.write_string(shape.m_name);
            compressor.write_numeric_value(shape.m_start);
            compressor.write_numeric_value(shape.m_size);
        }
    }
}

auto ParentRuleShapesArray::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<void> {
    log_shape_id_t num_log_shapes{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(num_log_shapes)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    m_shapes_per_log_shape.reserve(m_shapes_per_log_shape.size() + num_log_shapes);

    for (log_shape_id_t i{0}; i < num_log_shapes; ++i) {
        shape_size_t num_shapes{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(num_shapes)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        ParentRuleShapes shapes;
        shapes.reserve(num_shapes);
        for (shape_size_t j{0}; j < num_shapes; ++j) {
            shape_size_t name_size{};
            if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(name_size)) {
                return ClppErrorCode{ClppErrorCodeEnum::Failure};
            }
            std::string name;
            name.resize(name_size);
            if (clp_s::ErrorCodeSuccess
                != decompressor.try_read_exact_length(name.data(), name_size))
            {
                return ClppErrorCode{ClppErrorCodeEnum::Failure};
            }
            shape_size_t start{};
            if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(start)) {
                return ClppErrorCode{ClppErrorCodeEnum::Failure};
            }
            shape_size_t size{};
            if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
                return ClppErrorCode{ClppErrorCodeEnum::Failure};
            }
            shapes.emplace_back(name, start, size);
        }
        add_log_shape(std::move(shapes));
    }
    return ystdlib::error_handling::success();
}

auto ParentRuleShapesArray::add_log_shape(ParentRuleShapes shapes) -> void {
    if (m_shapes_per_log_shape.size() >= cMaxLogShapeId
        || false == shapes_fit_in_shape_size(shapes))
    {
        throw std::system_error{
                ystdlib::error_handling::make_error_code(
                        ClppErrorCode{ClppErrorCodeEnum::OutOfBounds}
                ),
                "clpp::ParentRuleShapesArray::add_log_shape: cMaxLogShapeId or cMaxShapeSize "
                "exceeded"
        };
    }
    m_data_size += get_shapes_data_size(shapes);
    m_shapes_per_log_shape.emplace_back(std::move(shapes));
}
}  // namespace clpp

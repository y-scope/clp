#include "ParentRuleShapes.hpp"

#include <cstddef>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto ParentRuleShapes::compress(clp_s::ZstdCompressor& compressor) const
        -> ystdlib::error_handling::Result<void> {
    compressor.write_numeric_value(m_parent_rule_shapes.size());
    for (auto const& shape : m_parent_rule_shapes) {
        compressor.write_numeric_value(shape.m_name.size());
        compressor.write_string(shape.m_name);
        compressor.write_numeric_value(shape.m_start);
        compressor.write_numeric_value(shape.m_size);
    }
    return ystdlib::error_handling::success();
}

auto ParentRuleShapes::decompress(clp_s::ZstdDecompressor& decompressor)
        -> ystdlib::error_handling::Result<ParentRuleShapes> {
    ParentRuleShapes shapes;
    size_t shapes_size{};
    if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(shapes_size)) {
        return ClppErrorCode{ClppErrorCodeEnum::Failure};
    }
    shapes.m_parent_rule_shapes.reserve(shapes_size);

    for (size_t i{0}; i < shapes_size; ++i) {
        size_t name_size{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(name_size)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        std::string name;
        name.resize(name_size);
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_exact_length(name.data(), name_size)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        size_t start{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(start)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        size_t size{};
        if (clp_s::ErrorCodeSuccess != decompressor.try_read_numeric_value(size)) {
            return ClppErrorCode{ClppErrorCodeEnum::Failure};
        }
        shapes.emplace_parent_rule_shape(name, start, size);
    }
    return shapes;
}
}  // namespace clpp

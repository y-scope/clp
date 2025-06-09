#include <string_view>

#include <catch2/catch_tostring.hpp>

#ifndef CATCH_CONFIG_CPP17_STRING_VIEW
namespace Catch {
template <>
struct StringMaker<std::string_view> {
    static std::string convert(std::string_view& value) { return std::string{value}; }
};
}  // namespace Catch
#endif

#include <string_view>

#include <catch2/catch_tostring.hpp>

namespace Catch {
template <typename T>
struct StringMaker<T> {
    static std::string convert(T const& value) { return std::string{value}; }
};
}  // namespace Catch

set(
        STRING_UTILS_HEADER_LIST
        "${CLP_SOURCE_DIR}/include/string_utils.hpp"
        "${CLP_SOURCE_DIR}/include/string_utils.inc"
)
add_library(
        string_utils
        string_utils.cpp
        ${STRING_UTILS_HEADER_LIST}
)
add_library(string_utils::string_utils ALIAS string_utils)
target_include_directories(string_utils PUBLIC ../include)
target_compile_features(string_utils PRIVATE cxx_std_17)

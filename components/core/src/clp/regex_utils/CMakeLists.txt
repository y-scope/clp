set(
        REGEX_UTILS_HEADER_LIST
        "regex_utils.hpp"
)
add_library(
        regex_utils
        regex_utils.cpp
        ${REGEX_UTILS_HEADER_LIST}
)
add_library(clp::regex_utils ALIAS regex_utils)
target_include_directories(regex_utils PUBLIC ../)
target_compile_features(regex_utils PRIVATE cxx_std_20)

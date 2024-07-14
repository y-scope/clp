set(
        REGEX_UTILS_HEADER_LIST
        "ErrorCode.hpp"
        "RegexToWildcardTranslatorConfig.hpp"
        "constants.hpp"
        "regex_utils.hpp"
)
add_library(
        regex_utils
        regex_utils_regex_to_wildcard.cpp
        regex_utils_anchors.cpp
        ErrorCode.cpp
        ${REGEX_UTILS_HEADER_LIST}
)
add_library(clp::regex_utils ALIAS regex_utils)
target_include_directories(regex_utils
        PUBLIC
        ../
        PRIVATE
        "${PROJECT_SOURCE_DIR}/submodules"
)
target_compile_features(regex_utils PRIVATE cxx_std_20)

set(
        REGEX_UTILS_HEADER_LIST
        "constants.hpp"
        "ErrorCode.hpp"
        "regex_translation_utils.hpp"
        "RegexToWildcardTranslatorConfig.hpp"
)
add_library(
        regex_utils
        ErrorCode.cpp
        regex_translation_utils.cpp
        ${REGEX_UTILS_HEADER_LIST}
)
add_library(clp::regex_utils ALIAS regex_utils)
target_include_directories(regex_utils
        PUBLIC
        ../
        "${PROJECT_SOURCE_DIR}/submodules"
)
target_link_libraries(regex_utils PRIVATE clp::string_utils)
target_compile_features(regex_utils PRIVATE cxx_std_20)

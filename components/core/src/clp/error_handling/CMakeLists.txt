set(
        ERROR_HANDLING_HEADER_LIST
        "ErrorCode.hpp"
)
add_library(error_handling INTERFACE)
add_library(clp::error_handling ALIAS error_handling)
target_sources(error_handling INTERFACE ${ERROR_HANDLING_HEADER_LIST})
target_include_directories(error_handling INTERFACE ../)
target_compile_features(error_handling INTERFACE cxx_std_20)

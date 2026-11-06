set(
    CLP_S_LOG_CONVERTER_SOURCES
    LogConverter.cpp
    LogConverter.hpp
    LogSerializer.cpp
    LogSerializer.hpp
)

set(
    CLP_S_LOG_CONVERTER_EXECUTABLE_SOURCES
    CommandLineArguments.cpp
    CommandLineArguments.hpp
    log_converter.cpp
)

if(CLP_BUILD_CLP_S_LOG_CONVERTER)
    add_library(
        clp_s_log_converter
        ${CLP_S_LOG_CONVERTER_SOURCES}
    )
    add_library(clp_s::log_converter ALIAS clp_s_log_converter)
    target_compile_features(clp_s_log_converter PRIVATE cxx_std_20)
    target_include_directories(clp_s_log_converter PUBLIC ../..)
    target_link_libraries(
        clp_s_log_converter
        PUBLIC
        clp_s::clp_dependencies
        clp_s::io
        log-surgeon::log-surgeon
        ystdlib::containers
        ystdlib::error_handling
        PRIVATE
        Boost::headers
        msgpack-cxx
        nlohmann_json::nlohmann_json
        spdlog::spdlog
    )
endif()

if(CLP_BUILD_TESTING)
    add_library(clp_s_log_converter_unit_test_sources INTERFACE)
    add_library(
        clp_s::log_converter::unit_test_sources
        ALIAS clp_s_log_converter_unit_test_sources
    )
    target_sources(
        clp_s_log_converter_unit_test_sources
        INTERFACE
        test/test-LogConverter.cpp
    )
endif()

if(CLP_BUILD_EXECUTABLES)
    add_executable(
        log-converter
        ${CLP_S_LOG_CONVERTER_EXECUTABLE_SOURCES}
    )
    target_compile_features(log-converter PRIVATE cxx_std_20)
    target_include_directories(log-converter PRIVATE ../../)
    target_link_libraries(
        log-converter
        PRIVATE
        Boost::program_options
        clp_s::io
        clp_s::log_converter
        fmt::fmt
        spdlog::spdlog
    )
    set_target_properties(
        log-converter
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
endif()

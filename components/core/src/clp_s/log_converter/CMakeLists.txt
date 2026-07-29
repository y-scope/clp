set(
    CLP_S_LOG_CONVERTER_SOURCES
    CommandLineArguments.cpp
    CommandLineArguments.hpp
    LogConverter.cpp
    LogConverter.hpp
    LogSerializer.cpp
    LogSerializer.hpp
)

if(CLP_BUILD_EXECUTABLES)
    add_executable(
        log-converter
        log_converter.cpp
        ${CLP_S_LOG_CONVERTER_SOURCES}
    )
    target_compile_features(log-converter PRIVATE cxx_std_20)
    target_include_directories(log-converter PRIVATE ../../)
    target_link_libraries(
        log-converter
        PRIVATE
        Boost::program_options
        clp_s::clp_dependencies
        clp_s::io
        fmt::fmt
        log_surgeon::log_surgeon
        msgpack-cxx
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        ystdlib::containers
        ystdlib::error_handling
    )
    set_target_properties(
        log-converter
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
endif()

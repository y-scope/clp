set(
    CLP_S_CONVERTER_SOURCES
    CommandLineArguments.cpp
    CommandLineArguments.cpp
)

if(CLP_BUILD_EXECUTABLES)
    add_executable(
        log-converter
        log_converter.cpp
        ${CLP_S_CONVERTER_SOURCES}
    )
    target_compile_features(log-converter PRIVATE cxx_std_20)
    target_link_libraries(
        log-converter
        PRIVATE
        Boost::program_options
        clp_s::io
        log_surgeon::log_surgeon
        msgpack-cxx
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        ystdlib::containers
    )
    set_target_properties(
        log-converter
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
endif()
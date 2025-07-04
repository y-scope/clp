# Build Target Options
option(
    CLP_BUILD_EXECUTABLES
    "Build all executables."
    ON
)

option(
    CLP_BUILD_TESTING
    "Build all tests."
    ON
)

option(
    CLP_BUILD_CLP_REGEX_UTILS
    "Build clp::regex_utils."
    ON
)

option(
    CLP_BUILD_CLP_STRING_UTILS
    "Build clp::string_utils."
    ON
)

option(
    CLP_BUILD_CLP_S_ARCHIVEREADER
    "Build clp_s::archive_reader."
    ON
)

option(
    CLP_BUILD_CLP_S_ARCHIVEWRITER
    "Build clp_s::archive_writer."
    ON
)

option(
    CLP_BUILD_CLP_S_CLP_DEPENDENCIES
    "BUILD clp_s::clp_dependencies."
    ON
)

option(
    CLP_BUILD_CLP_S_IO
    "Build clp_s::io."
    ON
)

option(
    CLP_BUILD_CLP_S_JSONCONSTRUCTOR
    "Build clp_s::json_constructor."
    ON
)

option(
    CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES
    "Build clp_s::reducer_dependencies."
    ON
)

option(
    CLP_BUILD_CLP_S_SEARCH
    "Build clp_s::search."
    ON
)

option(
    CLP_BUILD_CLP_S_SEARCH_AST
    "Build clp_s::search::ast."
    ON
)

option(
    CLP_BUILD_CLP_S_SEARCH_KQL
    "Build clp_s::search::kql."
    ON
)

option(
    CLP_BUILD_CLP_S_SEARCH_SQL
    "Build clp_s::search::sql."
    ON
)

option(
    CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    "Build clp_s::timestamp_pattern."
    ON
)

# Validates that the `CLP_BUILD_` options required by `TARGET_CLP_BUILD_OPTION` are `ON`.
#
# @param {string} TARGET_CLP_BUILD_OPTION
# @param {string[]} ARGN The required `CLP_BUILD_` options
function(validate_clp_dependencies_for_target TARGET_CLP_BUILD_OPTION)
    if (NOT DEFINED TARGET_CLP_BUILD_OPTION OR TARGET_CLP_BUILD_OPTION STREQUAL "")
        message(FATAL_ERROR "TARGET_CLP_BUILD_OPTION can't be unset or empty.")
    endif()

    foreach(DEPENDENCY IN LISTS ARGN)
        if (NOT "${${DEPENDENCY}}")
            message(FATAL_ERROR "${TARGET_CLP_BUILD_OPTION} requires ${DEPENDENCY}=ON")
        endif()
    endforeach()
endfunction()

# Sets the given `CLP_NEED_` flags as directory properties
#
# @param {string[]} ARGV The `CLP_NEED_` flags to set.
function(set_clp_need_flags)
    foreach(NEEDS_FLAG IN LISTS ARGV)
        set_property(DIRECTORY PROPERTY "${NEEDS_FLAG}" ON)
    endforeach()
endfunction()

function(validate_clp_binaries_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_EXECUTABLES
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_ARCHIVEREADER
        CLP_BUILD_CLP_S_ARCHIVEWRITER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_JSONCONSTRUCTOR
        CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES
        CLP_BUILD_CLP_S_SEARCH
        CLP_BUILD_CLP_S_SEARCH_AST
        CLP_BUILD_CLP_S_SEARCH_KQL
    )
endfunction()

function(set_clp_binaries_dependencies)
    set_clp_need_flags(
        CLP_NEED_ABSL
        CLP_NEED_BOOST
        CLP_NEED_CURL
        CLP_NEED_DATE
        CLP_NEED_FMT
        CLP_NEED_LIBARCHIVE
        CLP_NEED_LOG_SURGEON
        CLP_NEED_MARIADB
        CLP_NEED_MONGOCXX
        CLP_NEED_MSGPACKCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_OPENSSL
        CLP_NEED_SIMDJSON
        CLP_NEED_SPDLOG
        CLP_NEED_SQLITE
        CLP_NEED_YAMLCPP
        CLP_NEED_YSTDLIB
        CLP_NEED_ZSTD
    )
endfunction()

function(validate_clp_tests_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_TESTING
        CLP_BUILD_CLP_REGEX_UTILS
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_SEARCH_AST
        CLP_BUILD_CLP_S_SEARCH_KQL
        CLP_BUILD_CLP_S_SEARCH_SQL
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(set_clp_tests_dependencies)
    set_clp_need_flags(
        CLP_NEED_ABSL
        CLP_NEED_BOOST
        CLP_NEED_CATCH2
        CLP_NEED_DATE
        CLP_NEED_FMT
        CLP_NEED_LIBARCHIVE
        CLP_NEED_LOG_SURGEON
        CLP_NEED_LZMA
        CLP_NEED_MARIADB
        CLP_NEED_MONGOCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_OPENSSL
        CLP_NEED_SIMDJSON
        CLP_NEED_SPDLOG
        CLP_NEED_SQLITE
        CLP_NEED_YAMLCPP
        CLP_NEED_YSTDLIB
        CLP_NEED_ZSTD
    )
endfunction()

function(validate_clp_regex_utils_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_REGEX_UTILS
        CLP_BUILD_CLP_STRING_UTILS
    )
endfunction()

function(set_clp_regex_utils_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_YSTDLIB TRUE)
endfunction()

function(validate_clp_s_archivereader_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_ARCHIVEREADER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(set_clp_s_archivereader_dependencies)
    set_clp_need_flags(
        CLP_NEED_ABSL
        CLP_NEED_BOOST
        CLP_NEED_CURL
        CLP_NEED_FMT
        CLP_NEED_MSGPACKCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_archivewriter_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_ARCHIVEWRITER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(set_clp_s_archivewriter_dependencies)
    set_clp_need_flags(
        CLP_NEED_ABSL
        CLP_NEED_BOOST
        CLP_NEED_CURL
        CLP_NEED_FMT
        CLP_NEED_MSGPACKCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_SIMDJSON
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_clp_dependencies_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_STRING_UTILS
    )
endfunction()

function(set_clp_s_clp_dependencies_dependencies)
    set_clp_need_flags(
        CLP_NEED_BOOST
        CLP_NEED_CURL
        CLP_NEED_FMT
        CLP_NEED_MSGPACKCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_OPENSSL
        CLP_NEED_SPDLOG
        CLP_NEED_YSTDLIB
        CLP_NEED_ZSTD
    )
endfunction()

function(validate_clp_s_io_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
    )
endfunction()

function(set_clp_s_io_dependencies)
    set_clp_need_flags(
        CLP_NEED_BOOST
        CLP_NEED_FMT
        CLP_NEED_SPDLOG
        CLP_NEED_ZSTD
    )
endfunction()

function(validate_clp_s_json_constructor_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_JSONCONSTRUCTOR
        CLP_BUILD_CLP_S_ARCHIVEREADER
    )
endfunction()

function(set_clp_s_json_constructor_dependencies)
    set_clp_need_flags(
        CLP_NEED_FMT
        CLP_NEED_MONGOCXX
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_reducer_dependencies_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
    )
endfunction()

function(set_clp_s_reducer_dependencies_dependencies)
    set_clp_need_flags(
        CLP_NEED_NLOHMANN_JSON
    )
endfunction()

function(validate_clp_s_search_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH
        CLP_BUILD_CLP_S_ARCHIVEREADER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(set_clp_s_search_dependencies)
    set_clp_need_flags(
        CLP_NEED_ABSL
        CLP_NEED_SIMDJSON
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_search_ast_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_AST
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(set_clp_s_search_ast_dependencies)
    set_clp_need_flags(
        CLP_NEED_SIMDJSON
    )
endfunction()

function(validate_clp_s_search_kql_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_KQL
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(set_clp_s_search_kql_dependencies)
    set_clp_need_flags(
        CLP_NEED_ANTLR
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_search_sql_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_SQL
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(set_clp_s_search_sql_dependencies)
    set_clp_need_flags(
        CLP_NEED_ANTLR
        CLP_NEED_SPDLOG
    )
endfunction()

function(validate_clp_s_timestamppattern_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_TIMESTAMPPATTERN
        CLP_BUILD_CLP_STRING_UTILS
    )
endfunction()

function(set_clp_s_timestamppattern_dependencies)
    set_clp_need_flags(
        CLP_NEED_DATE
        CLP_NEED_SPDLOG
    )
endfunction()

# Validates that for each target whose `CLP_BUILD_` option is `ON`, the `CLP_BUILD_` options for
# the target's dependencies are also `ON`; Sets the required `CLP_NEED_` flags for any target that
# will be built.
function(validate_and_setup_all_clp_dependency_flags)
    if (CLP_BUILD_EXECUTABLES)
        validate_clp_binaries_dependencies()
        set_clp_binaries_dependencies()
    endif()

    if (CLP_BUILD_TESTING)
        validate_clp_tests_dependencies()
        set_clp_tests_dependencies()
    endif()

    if (CLP_BUILD_CLP_REGEX_UTILS)
        validate_clp_regex_utils_dependencies()
        set_clp_regex_utils_dependencies()
    endif()

    # clp::string_utils has no dependencies

    if (CLP_BUILD_CLP_S_ARCHIVEREADER)
        validate_clp_s_archivereader_dependencies()
        set_clp_s_archivereader_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_ARCHIVEWRITER)
        validate_clp_s_archivewriter_dependencies()
        set_clp_s_archivewriter_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        validate_clp_s_clp_dependencies_dependencies()
        set_clp_s_clp_dependencies_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_IO)
        validate_clp_s_io_dependencies()
        set_clp_s_io_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_JSONCONSTRUCTOR)
        validate_clp_s_json_constructor_dependencies()
        set_clp_s_json_constructor_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES)
        validate_clp_s_reducer_dependencies_dependencies()
        set_clp_s_reducer_dependencies_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_SEARCH)
        validate_clp_s_search_dependencies()
        set_clp_s_search_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_SEARCH_AST)
        validate_clp_s_search_ast_dependencies()
        set_clp_s_search_ast_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_SEARCH_KQL)
        validate_clp_s_search_kql_dependencies()
        set_clp_s_search_kql_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_SEARCH_SQL)
        validate_clp_s_search_sql_dependencies()
        set_clp_s_search_sql_dependencies()
    endif()

    if (CLP_BUILD_CLP_S_TIMESTAMPPATTERN)
        validate_clp_s_timestamppattern_dependencies()
        set_clp_s_timestamppattern_dependencies()
    endif()
endfunction()

function (convert_clp_dependency_properties_to_variables)
    list(APPEND CLP_NEED_FLAGS
        CLP_NEED_ABSL
        CLP_NEED_ANTLR
        CLP_NEED_BOOST
        CLP_NEED_CATCH2
        CLP_NEED_CURL
        CLP_NEED_DATE
        CLP_NEED_FMT
        CLP_NEED_LOG_SURGEON
        CLP_NEED_LIBARCHIVE
        CLP_NEED_LZMA
        CLP_NEED_MARIADB
        CLP_NEED_MONGOCXX
        CLP_NEED_MSGPACKCXX
        CLP_NEED_NLOHMANN_JSON
        CLP_NEED_OPENSSL
        CLP_NEED_SIMDJSON
        CLP_NEED_SPDLOG
        CLP_NEED_SQLITE
        CLP_NEED_YAMLCPP
        CLP_NEED_YSTDLIB
        CLP_NEED_ZSTD
    )

    foreach(FLAG IN LISTS CLP_NEED_FLAGS)
        get_property(VALUE DIRECTORY PROPERTY "${FLAG}")
        set("${FLAG}" "${VALUE}" PARENT_SCOPE)
    endforeach()
endfunction()

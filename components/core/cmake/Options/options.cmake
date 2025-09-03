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
    if(NOT DEFINED TARGET_CLP_BUILD_OPTION OR TARGET_CLP_BUILD_OPTION STREQUAL "")
        message(FATAL_ERROR "TARGET_CLP_BUILD_OPTION can't be unset or empty.")
    endif()

    # Only validate dependencies for this option if the option is enabled.
    if(NOT "${${TARGET_CLP_BUILD_OPTION}}")
        return()
    endif()

    foreach(DEPENDENCY IN LISTS ARGN)
        if(NOT "${${DEPENDENCY}}")
            message(FATAL_ERROR "${TARGET_CLP_BUILD_OPTION} requires ${DEPENDENCY}=ON")
        endif()
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
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(validate_clp_s_archivewriter_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_ARCHIVEWRITER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(validate_clp_s_clp_dependencies_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_STRING_UTILS
    )
endfunction()

function(validate_clp_s_io_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_IO
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
    )
endfunction()

function(validate_clp_s_json_constructor_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_JSONCONSTRUCTOR
        CLP_BUILD_CLP_S_ARCHIVEREADER
    )
endfunction()

function(validate_clp_s_reducer_dependencies_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
    )
endfunction()

function(validate_clp_s_search_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_ARCHIVEREADER
        CLP_BUILD_CLP_S_CLP_DEPENDENCIES
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(validate_clp_s_search_ast_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_AST
        CLP_BUILD_CLP_S_TIMESTAMPPATTERN
    )
endfunction()

function(validate_clp_s_search_kql_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_KQL
        CLP_BUILD_CLP_STRING_UTILS
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(validate_clp_s_search_sql_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_SEARCH_SQL
        CLP_BUILD_CLP_S_SEARCH_AST
    )
endfunction()

function(validate_clp_s_timestamppattern_dependencies)
    validate_clp_dependencies_for_target(CLP_BUILD_CLP_S_TIMESTAMPPATTERN
        CLP_BUILD_CLP_STRING_UTILS
    )
endfunction()

# Validates that for each target whose `CLP_BUILD_` option is `ON`, the `CLP_BUILD_` options for
# the target's dependencies are also `ON`.
function(validate_all_clp_dependency_flags)
    validate_clp_binaries_dependencies()
    validate_clp_tests_dependencies()
    validate_clp_regex_utils_dependencies()
    # clp::string_utils has no dependencies

    validate_clp_s_archivereader_dependencies()
    validate_clp_s_archivewriter_dependencies()
    validate_clp_s_clp_dependencies_dependencies()
    validate_clp_s_io_dependencies()
    validate_clp_s_json_constructor_dependencies()
    validate_clp_s_reducer_dependencies_dependencies()
    validate_clp_s_search_dependencies()
    validate_clp_s_search_ast_dependencies()
    validate_clp_s_search_kql_dependencies()
    validate_clp_s_search_sql_dependencies()
    validate_clp_s_timestamppattern_dependencies()
endfunction()

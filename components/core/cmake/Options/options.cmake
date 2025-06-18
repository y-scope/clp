# Build Target Options
option(
    CLP_BUILD_BINARIES
    "Build all binaries."
    ON
)

option(
    CLP_BUILD_TESTS
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

function(validate_clp_binaries_dependencies)
    if (NOT CLP_BUILD_CLP_REGEX_UTILS)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_REGEX_UTILS=ON")
    endif()

    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_ARCHIVEREADER)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_ARCHIVEREADER=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_ARCHIVEWRITER)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_ARCHIVEWRITER=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_IO)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_IO=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_JSONCONSTRUCTOR)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_JSONCONSTRUCTOR=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_AST)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH_AST=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_KQL)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH_KQL=ON")
    endif()
endfunction()

function(set_clp_binaries_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_DATE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_LOG_SURGEON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_LIBARCHIVE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MARIADB TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MONGOCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_OPENSSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SIMDJSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SQLITE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_YAMLCPP TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_YSTDLIB TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_ZSTD TRUE)
endfunction()

function(validate_clp_tests_dependencies)
    if (NOT CLP_BUILD_CLP_REGEX_UTILS)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_REGEX_UTILS=ON")
    endif()

    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_AST)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH_AST=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_KQL)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH_KQL=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_SQL)
        message(FATAL_ERROR "CLP_BUILD_BINARIES requires CLP_BUILD_CLP_S_SEARCH_SQL=ON")
    endif()
endfunction()

function(set_clp_tests_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ABSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_CATCH2 TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_DATE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_LOG_SURGEON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_LIBARCHIVE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_LZMA TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MARIADB TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MONGOCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_OPENSSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SIMDJSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SQLITE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_YAMLCPP TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_YSTDLIB TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_ZSTD TRUE)
endfunction()

function(validate_clp_regex_utils_dependencies)
    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_CLP_REGEX_UTILS requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()
endfunction()

function(set_clp_regex_utils_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_YSTDLIB TRUE)
endfunction()

function(validate_clp_s_archivereader_dependencies)
    if (NOT CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEREADER requires CLP_BUILD_CLP_S_CLP_DEPENDENCIES=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_IO)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEREADER requires CLP_BUILD_CLP_S_IO=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_TIMESTAMPPATTERN)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEREADER requires CLP_BUILD_CLP_S_TIMESTAMPPATTERN=ON")
    endif()
endfunction()

function(set_clp_s_archivereader_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ABSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_CURL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_clp_s_archivewriter_dependencies)
    if (NOT CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEWRITER requires CLP_BUILD_CLP_S_CLP_DEPENDENCIES=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_IO)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEWRITER requires CLP_BUILD_CLP_S_IO=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_TIMESTAMPPATTERN)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_ARCHIVEWRITER requires CLP_BUILD_CLP_S_TIMESTAMPPATTERN=ON")
    endif()
endfunction()

function(set_clp_s_archivewriter_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ABSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_CURL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SIMDJSON)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_clp_s_clp_dependencies_dependencies)
    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_CLP_DEPENDENCIES requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()
endfunction()

function(set_clp_s_clp_dependencies_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_CURL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_OPENSSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_YSTDLIB TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_ZSTD TRUE)
endfunction()

function(validate_clp_s_io_dependencies)
    if (NOT CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_IO requires CLP_BUILD_CLP_S_CLP_DEPENDENCIES=ON")
    endif()
endfunction()

function(set_clp_s_io_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_BOOST TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_ZSTD TRUE)
endfunction()

function(validate_clp_s_json_constructor_dependencies)
    if (NOT CLP_BUILD_CLP_S_ARCHIVEREADER)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_JSONCONSTRUCTOR requires CLP_BUILD_CLP_S_ARCHIVEREADER=ON")
    endif()
endfunction()

function(set_clp_s_json_constructor_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_FMT TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_MONGOCXX TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_clp_s_reducer_dependencies_dependencies)
    if (NOT CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_REDUCER_DEPENDENCIES requires CLP_BUILD_CLP_S_CLP_DEPENDENCIES=ON")
    endif()
endfunction()

function(set_clp_s_reducer_dependencies_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
endfunction()

function(validate_clp_s_search_dependencies)
    if (NOT CLP_BUILD_CLP_S_ARCHIVEREADER)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH requires CLP_BUILD_CLP_S_ARCHIVEREADER=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_CLP_DEPENDENCIES)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH requires CLP_BUILD_CLP_S_CLP_DEPENDENCIES=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_AST)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH requires CLP_BUILD_CLP_S_SEARCH_AST=ON")
    endif()
endfunction()

function(set_clp_s_search_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ABSL TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SIMDJSON TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(set_clp_s_reducer_dependencies_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON TRUE)
endfunction()

function(validate_clp_s_search_ast_dependencies)
    if (NOT CLP_BUILD_CLP_S_TIMESTAMPPATTERN)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH_AST requires CLP_BUILD_CLP_S_TIMESTAMPPATTERN=ON")
    endif()
endfunction()

function(set_clp_s_search_ast_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_SIMDJSON TRUE)
endfunction()

function(validate_clp_s_search_kql_dependencies)
    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH_KQL requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()

    if (NOT CLP_BUILD_CLP_S_SEARCH_AST)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH_KQL requires CLP_BUILD_CLP_S_SEARCH_AST=ON")
    endif()
endfunction()

function(set_clp_s_search_kql_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ANTLR TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_clp_s_search_sql_dependencies)
    if (NOT CLP_BUILD_CLP_S_SEARCH_AST)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_SEARCH_SQL requires CLP_BUILD_CLP_S_SEARCH_AST=ON")
    endif()
endfunction()

function(set_clp_s_search_sql_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_ANTLR TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_clp_s_timestamppattern_dependencies)
    if (NOT CLP_BUILD_CLP_STRING_UTILS)
        message(FATAL_ERROR "CLP_BUILD_CLP_S_TIMESTAMPPATTERN requires CLP_BUILD_CLP_STRING_UTILS=ON")
    endif()
endfunction()

function(set_clp_s_timestamppattern_dependencies)
    set_property(DIRECTORY PROPERTY CLP_NEED_DATE TRUE)
    set_property(DIRECTORY PROPERTY CLP_NEED_SPDLOG TRUE)
endfunction()

function(validate_and_setup_all_clp_dependency_flags)
    if (CLP_BUILD_BINARIES)
        validate_clp_binaries_dependencies()
        set_clp_binaries_dependencies()
    endif()

    if (CLP_BUILD_TESTS)
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
    get_property(CLP_NEED_ABSL DIRECTORY PROPERTY CLP_NEED_ABSL)
    set(CLP_NEED_ABSL "${CLP_NEED_ABSL}" PARENT_SCOPE)
    get_property(CLP_NEED_ANTLR DIRECTORY PROPERTY CLP_NEED_ANTLR)
    set(CLP_NEED_ANTLR "${CLP_NEED_ANTLR}" PARENT_SCOPE)
    get_property(CLP_NEED_BOOST DIRECTORY PROPERTY CLP_NEED_BOOST)
    set(CLP_NEED_BOOST "${CLP_NEED_BOOST}" PARENT_SCOPE)
    get_property(CLP_NEED_CATCH2 DIRECTORY PROPERTY CLP_NEED_CATCH2)
    set(CLP_NEED_CATCH2 "${CLP_NEED_CATCH2}" PARENT_SCOPE)
    get_property(CLP_NEED_CURL DIRECTORY PROPERTY CLP_NEED_CURL)
    set(CLP_NEED_CURL "${CLP_NEED_CURL}" PARENT_SCOPE)
    get_property(CLP_NEED_DATE DIRECTORY PROPERTY CLP_NEED_DATE)
    set(CLP_NEED_DATE "${CLP_NEED_DATE}" PARENT_SCOPE)
    get_property(CLP_NEED_FMT DIRECTORY PROPERTY CLP_NEED_FMT)
    set(CLP_NEED_FMT "${CLP_NEED_FMT}" PARENT_SCOPE)
    get_property(CLP_NEED_LOG_SURGEON DIRECTORY PROPERTY CLP_NEED_LOG_SURGEON)
    set(CLP_NEED_LOG_SURGEON "${CLP_NEED_LOG_SURGEON}" PARENT_SCOPE)
    get_property(CLP_NEED_LIBARCHIVE DIRECTORY PROPERTY CLP_NEED_LIBARCHIVE)
    set(CLP_NEED_LIBARCHIVE "${CLP_NEED_LIBARCHIVE}" PARENT_SCOPE)
    get_property(CLP_NEED_LZMA DIRECTORY PROPERTY CLP_NEED_LZMA)
    set(CLP_NEED_LZMA "${CLP_NEED_LZMA}" PARENT_SCOPE)
    get_property(CLP_NEED_MARIADB DIRECTORY PROPERTY CLP_NEED_MARIADB)
    set(CLP_NEED_MARIADB "${CLP_NEED_MARIADB}" PARENT_SCOPE)
    get_property(CLP_NEED_MONGOCXX DIRECTORY PROPERTY CLP_NEED_MONGOCXX)
    set(CLP_NEED_MONGOCXX "${CLP_NEED_MONGOCXX}" PARENT_SCOPE)
    get_property(CLP_NEED_MSGPACKCXX DIRECTORY PROPERTY CLP_NEED_MSGPACKCXX)
    set(CLP_NEED_MSGPACKCXX "${CLP_NEED_MSGPACKCXX}" PARENT_SCOPE)
    get_property(CLP_NEED_NLOHMANN_JSON DIRECTORY PROPERTY CLP_NEED_NLOHMANN_JSON)
    set(CLP_NEED_NLOHMANN_JSON "${CLP_NEED_NLOHMANN_JSON}" PARENT_SCOPE)
    get_property(CLP_NEED_OPENSSL DIRECTORY PROPERTY CLP_NEED_OPENSSL)
    set(CLP_NEED_OPENSSL "${CLP_NEED_OPENSSL}" PARENT_SCOPE)
    get_property(CLP_NEED_SIMDJSON DIRECTORY PROPERTY CLP_NEED_SIMDJSON)
    set(CLP_NEED_SIMDJSON "${CLP_NEED_SIMDJSON}" PARENT_SCOPE)
    get_property(CLP_NEED_SPDLOG DIRECTORY PROPERTY CLP_NEED_SPDLOG)
    set(CLP_NEED_SPDLOG "${CLP_NEED_SPDLOG}" PARENT_SCOPE)
    get_property(CLP_NEED_SQLITE DIRECTORY PROPERTY CLP_NEED_SQLITE)
    set(CLP_NEED_SQLITE "${CLP_NEED_SQLITE}" PARENT_SCOPE)
    get_property(CLP_NEED_YAMLCPP DIRECTORY PROPERTY CLP_NEED_YAMLCPP)
    set(CLP_NEED_YAMLCPP "${CLP_NEED_YAMLCPP}" PARENT_SCOPE)
    get_property(CLP_NEED_YSTDLIB DIRECTORY PROPERTY CLP_NEED_YSTDLIB)
    set(CLP_NEED_YSTDLIB "${CLP_NEED_YSTDLIB}" PARENT_SCOPE)
    get_property(CLP_NEED_ZSTD DIRECTORY PROPERTY CLP_NEED_ZSTD)
    set(CLP_NEED_ZSTD "${CLP_NEED_ZSTD}" PARENT_SCOPE)
endfunction()

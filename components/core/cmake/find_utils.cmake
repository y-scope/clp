# This file contains utility functions for finding dependencies.

# @param {string} LIBRARY
# @return {bool} Whether we have already run the find function for LIBRARY in the CLP_CHECKED_FIND
# variable.
function(get_clp_checked_find LIBRARY)
    get_property(CLP_CHECKED_FIND 
        DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTY "clp_checked_find_${LIBRARY}"
    )
    set(CLP_CHECKED_FIND "${CLP_CHECKED_FIND}" PARENT_SCOPE)
endfunction()

# Sets a flag indicating that we have run the find function for LIBRARY.
# @param {string} LIBRARY
function(set_clp_checked_find LIBRARY)
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY "clp_checked_find_${LIBRARY}" TRUE)
endfunction()

# Finds and sets up the the absl library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_absl)
    find_package(absl REQUIRED)
endmacro()

# Finds and sets up the the antlr4-runtime library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_antlr4_runtime)
    find_package(antlr4-runtime REQUIRED)
endmacro()

# Finds and sets up the the Boost library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_boost)
    get_clp_checked_find(boost)
    if(CLP_CHECKED_FIND)
        # Silence output from `find_package(Boost)` on repeated invocations.
        set(CLP_QUIET_FIND_BOOST QUIET)
    endif()

    if(CLP_USE_STATIC_LIBS)
        set(Boost_USE_STATIC_LIBS ON)
    endif()

    find_package(
        Boost
        1.81
        ${CLP_QUIET_FIND_BOOST}
        REQUIRED
            filesystem
            iostreams
            program_options
            regex
            system
            url
    )
    if(Boost_VERSION VERSION_GREATER 1.88)
        message(
            FATAL_ERROR
            "Boost version ${Boost_VERSION} is newer than the maximum allowed version (1.88.0)."
        )
    endif()
    set_clp_checked_find(boost)
endmacro()

# Finds and sets up Catch2.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_catch2)
    find_package(Catch2 3.8.0 REQUIRED)
endmacro()

# Finds and sets up libcurl.
# By default, CURL does not provide static libraries.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_curl)
    find_package(CURL 7.61.1 REQUIRED)
endmacro()

# Finds and sets up the date library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_date)
    find_package(date REQUIRED)
endmacro()

# Finds and sets up the fmt library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_fmt)
    find_package(fmt REQUIRED)
endmacro()

# Finds and sets up libarchive.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_libarchive)
    if(CLP_USE_STATIC_LIBS)
        set(LibArchive_USE_STATIC_LIBS ON)
    endif()
    find_package(LibArchive REQUIRED)
endmacro()

# Finds and sets up the log_surgeon library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_log_surgeon)
    find_package(log_surgeon REQUIRED)
endmacro()

# Finds and sets up the LZMA library.
# TODO: Add a script in ./cmake/Modules to properly import LZMA in find_package()'s module mode.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_lzma)
    if(CLP_USE_STATIC_LIBS)
        set(LibLZMA_ROOT ${LibLZMA-static_ROOT})
        set(LibLZMA_USE_STATIC_LIBS ON)
    else()
        set(LibLZMA_ROOT ${LibLZMA-shared_ROOT})
    endif()
    # Version 5.8.1 and above address CVE-2024-3094 and CVE-2025-31115.
    find_package(LibLZMA 5.8.1 REQUIRED)
endmacro()

# Finds and sets up the MariaDBClient library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_mariadb_client)
    # Only log static linking warning one time
    get_clp_checked_find(mariadb_client)
    if(CLP_USE_STATIC_LIBS AND NOT CLP_CHECKED_FIND)
        # NOTE: We can't statically link to MariaDBClient since it's GPL
        message(AUTHOR_WARNING "MariaDBClient cannot be statically linked due to its license.")
    endif()
    find_package(MariaDBClient 3.1.0 REQUIRED)
    set_clp_checked_find(mariadb_client)
endmacro()

# Finds and sets up the mongocxx library.
# @return The name of the mongocxx target in the `MONGOCXX_TARGET` variable, and
# forwards any variables from the `find_package` call.
macro(clp_find_mongocxx)
    find_package(mongocxx REQUIRED)
    if(CLP_USE_STATIC_LIBS)
        set(MONGOCXX_TARGET mongo::mongocxx_static)
    else()
        set(MONGOCXX_TARGET mongo::mongocxx_shared)
    endif()
endmacro()

# Finds and sets up the msgpack library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_msgpack)
    find_package(msgpack-cxx 7.0.0 REQUIRED)
endmacro()

# Finds and sets up the nlohmann json library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endmacro()

# Finds and sets up OpenSSL.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_open_ssl)
    find_package(OpenSSL REQUIRED)
endmacro()

# Finds and sets up the simdjson library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_simdjson)
    find_package(simdjson REQUIRED)
endmacro()

# Finds and sets up the spdlog library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_spdlog)
    find_package(spdlog REQUIRED)
endmacro()

# Finds and sets up the sqlite library.
# @return Forwards any variables from the `FindDynamicLibraryDependencies` call.
macro(clp_find_sqlite)
    set(sqlite_DYNAMIC_LIBS "dl;m;pthread")
    include("${PROJECT_SOURCE_DIR}/cmake/Modules/FindLibraryDependencies.cmake")
    FindDynamicLibraryDependencies(sqlite "${sqlite_DYNAMIC_LIBS}")
endmacro()

# Finds and sets up the yaml-cpp library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_yaml_cpp)
    find_package(yaml-cpp REQUIRED)
endmacro()


# Finds and sets up ystdlib.
function(clp_find_ystdlib)
    # We can not call `add_subdirectory` for the same directory multiple times.
    get_clp_checked_find(ystdlib)
    if(CLP_CHECKED_FIND)
        return()
    endif()

    set(YSTDLIB_CPP_BUILD_TESTING OFF)
    add_subdirectory("${CLP_YSTDLIB_SOURCE_DIRECTORY}" "${CMAKE_BINARY_DIR}/ystdlib" EXCLUDE_FROM_ALL)
    set_clp_checked_find(ystdlib)
endfunction()

# Finds and sets up the ZStd library.
# @return Forwards any variables from the `find_package` call.
macro(clp_find_zstd)
    # v1.4.8 is the lowest version available in the package managers of the OSes we support.
    find_package(zstd 1.4.8 REQUIRED)
    if(CLP_USE_STATIC_LIBS)
        set(zstd_TARGET zstd::libzstd_static)
    else()
        set(zstd_TARGET zstd::libzstd_shared)
    endif()
endmacro()

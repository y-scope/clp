# This file contains utility functions for our other cmake scripts.

# Find and setup abseil library.
function(find_absl)
    find_package(absl REQUIRED)
    if (absl_FOUND)
        message(STATUS "Found absl ${absl_VERSION}")
    endif()
endfunction()

# Find and setup ANTLR library.
function(find_antlr)
    find_package(antlr4-runtime REQUIRED)
    if (antlr4-runtime_FOUND)
        message(STATUS "Found antlr4-runtime ${antlr4-runtime_VERSION}")
    endif()
endfunction()

# Find and setup Boost library.
function(find_boost)
    if(CLP_USE_STATIC_LIBS)
        set(Boost_USE_STATIC_LIBS ON)
    endif()
    find_package(Boost 1.81 REQUIRED iostreams program_options filesystem system regex url)
    if(Boost_FOUND)
        message(STATUS "Found Boost ${Boost_VERSION}")
    else()
        message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for Boost")
    endif()
endfunction()

# Find and setup Catch2 library.
function(find_catch2)
    find_package(Catch2 REQUIRED)
    if (Catch2_FOUND)
        message(STATUS "Found Catch2 ${Catch2_VERSION}")
    endif()
endfunction()

# Find and setup libcurl.
# By default, CURL does not provide static libraries.
# @return The list of libraries needed to link against CURL in the CURL_LIBRARIES variable.
function(find_curl)
    find_package(CURL 7.61.1 REQUIRED)
    if(CURL_FOUND)
        message(STATUS "Found CURL ${CURL_VERSION_STRING}")
        set(CURL_LIBRARIES ${CURL_LIBRARIES} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for CURL")
    endif()
endfunction()

# Find and setup date library.
function(find_date)
    find_package(date REQUIRED)
    if (date_FOUND)
        message(STATUS "Found date ${date_VERSION}")
    endif()
endfunction()

# Find and setup fmt library.
function(find_fmt)
    if(CLP_NEED_FMT)
        find_package(fmt REQUIRED)
        if(fmt_FOUND)
            message(STATUS "Found fmt ${fmt_VERSION}")
        endif()
    endif()
endfunction()

# Find and setup libarchive.
function(find_libarchive)
    if(CLP_USE_STATIC_LIBS)
        set(LibArchive_USE_STATIC_LIBS ON)
    endif()
    find_package(LibArchive REQUIRED)
    if(LibArchive_FOUND)
        message(STATUS "Found LibArchive ${LibArchive_VERSION}")
    endif()
endfunction()

# Find and setup log_surgeon library.
function(find_log_surgeon)
    find_package(log_surgeon REQUIRED)
    if(log_surgeon_FOUND)
        message(STATUS "Found log_surgeon ${log_surgeon_VERSION}")
    endif()
endfunction()

# Find and setup LZMA library.
# TODO: Add a script in ./cmake/Modules to properly import LZMA in find_package()'s module mode.
function(find_lzma)
    if(CLP_USE_STATIC_LIBS)
        set(LIBLZMA_USE_STATIC_LIBS ON)
    endif()
    find_package(LibLZMA REQUIRED)
    if(LIBLZMA_FOUND)
        message(STATUS "Found Lzma ${LIBLZMA_VERSION_STRING}")
        message(STATUS "Lzma library location: ${LIBLZMA_LIBRARIES}")
        message(STATUS "Lzma Include Dir: ${LIBLZMA_INCLUDE_DIRS}")

        # Version 5.8.1 and above address CVE-2024-3094 and CVE-2025-31115.
        set(REQUIRED_LIBLZMA_VERSION "5.8.1")
        if(LIBLZMA_VERSION_STRING VERSION_LESS ${REQUIRED_LIBLZMA_VERSION})
            message(
                FATAL_ERROR
                "Detected LibLZMA version ${LIBLZMA_VERSION_STRING} is older than required"
                " ${REQUIRED_LIBLZMA_VERSION}"
            )
        endif()
    else()
        message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for Lzma")
    endif()
    include_directories(${LIBLZMA_INCLUDE_DIRS})
endfunction()


# Find and setup MariaDBClient library.
function(find_mariadb)
    if(CLP_USE_STATIC_LIBS)
        # NOTE: We can't statically link to MariaDBClient since it's GPL
        message(AUTHOR_WARNING "MariaDBClient cannot be statically linked due to its license.")
    endif()
    find_package(MariaDBClient 3.1.0 REQUIRED)
    if(MariaDBClient_FOUND)
        message(STATUS "Found MariaDBClient ${MariaDBClient_VERSION}")
    else()
        message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for MariaDBClient")
    endif()
endfunction()

# Find and setup mongocxx library.
# @return The name of the mongocxx target in the `MONGOCXX_TARGET` variable.
function(find_mongocxx)
    find_package(mongocxx REQUIRED)
    message(STATUS "Found mongocxx ${mongocxx_VERSION}")
    if(CLP_USE_STATIC_LIBS)
        set(MONGOCXX_TARGET mongo::mongocxx_static PARENT_SCOPE)
    else()
        set(MONGOCXX_TARGET mongo::mongocxx_shared PARENT_SCOPE)
    endif()
endfunction()

# Find and setup msgpack library.
function(find_msgpack)
    find_package(msgpack-cxx 7.0.0 REQUIRED)
    if(msgpack-cxx_FOUND)
        message(STATUS "Found msgpack-cxx ${msgpack-cxx_VERSION}")
    else()
        message(FATAL_ERROR "Could not find msgpack-cxx")
    endif()
endfunction()

# Find and setup nlohmann_json library.
function(find_nlohmann_json)
    find_package(nlohmann_json REQUIRED)
    if(nlohmann_json_FOUND)
        message(STATUS "Found nlohmann_json ${nlohmann_json_VERSION}")
    endif()
endfunction()

# Find and setup OpenSSL library.
function(find_openssl)
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        message(STATUS "Found OpenSSL (${OPENSSL_VERSION})")
    else()
        message(FATAL_ERROR "OpenSSL not found")
    endif()
endfunction()

# Find and setup simdjson library.
function(find_simdjson)
    find_package(simdjson REQUIRED)
    if(simdjson_FOUND)
        message(STATUS "Found simdjson ${simdjson_VERSION}")
    endif()
endfunction()

# Find and setup spdlog library.
function(find_spdlog)
    find_package(spdlog REQUIRED)
    if(spdlog_FOUND)
        message(STATUS "Found spdlog ${spdlog_VERSION}")
    endif()
endfunction()

# Find and setup sqlite library.
function(find_sqlite)
    set(sqlite_DYNAMIC_LIBS "dl;m;pthread")
    include(cmake/Modules/FindLibraryDependencies.cmake)
    FindDynamicLibraryDependencies(sqlite "${sqlite_DYNAMIC_LIBS}")
endfunction()

# Find and setup yamlcpp library.
function(find_yamlcpp)
    find_package(yaml-cpp REQUIRED)
    if(yaml-cpp_FOUND)
        message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION}")
    endif()
endfunction()

# Find and setup ystdlib.
function(find_ystdlib)
    set(YSTDLIB_CPP_BUILD_TESTING OFF)
    add_subdirectory("${CLP_YSTDLIB_SOURCE_DIRECTORY}" "${CMAKE_BINARY_DIR}/ystdlib" EXCLUDE_FROM_ALL)
endfunction()

# Find and setup ZStd library.
function(find_zstd)
    if(CLP_USE_STATIC_LIBS)
        set(ZStd_USE_STATIC_LIBS ON)
    endif()
    find_package(ZStd 1.4.4 REQUIRED)
    if(ZStd_FOUND)
        message(STATUS "Found ZStd ${ZStd_VERSION}")
    else()
        message(FATAL_ERROR "Could not find ${CLP_LIBS_STRING} libraries for ZStd")
    endif()
endfunction()

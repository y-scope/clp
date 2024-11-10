# Try to find MariaDBClient
# NOTE: The FindMariaDBClient.cmake included with CMake has no support for static libraries, so we use our own.
#
# Set MariaDBClient_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done this will define:
#  MariaDBClient_FOUND - Whether MariaDBClient was found on the system
#  MariaDBClient_INCLUDE_DIR - The MariaDBClient include directories
#  MariaDBClient_VERSION - The version of MariaDBClient installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "mariadbclient_"
# - Variables that should be externally visible are prefixed with "MariaDBClient_"

set(mariadbclient_LIBNAME "mariadb")

include(cmake/Modules/FindLibraryDependencies.cmake)

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(mariadbclient_PKGCONF QUIET "lib${mariadbclient_LIBNAME}")

if(NOT mariadbclient_PKGCONF_FOUND AND APPLE)
    execute_process(
        COMMAND brew --prefix mariadb-connector-c
        RESULT_VARIABLE mariadbclient_BREW_RESULT
        OUTPUT_VARIABLE mariadbclient_MACOS_PREFIX
    )
    if(NOT mariadbclient_BREW_RESULT EQUAL 0)
        message(
            FATAL_ERROR
            "pkg-config cannot find ${mariadbclient_LIBNAME} and mariadb-connector-c isn't"
            " installed via Homebrew"
        )
    endif()
    string(STRIP "${mariadbclient_MACOS_PREFIX}" mariadbclient_MACOS_PREFIX)
    list(PREPEND CMAKE_PREFIX_PATH ${mariadbclient_MACOS_PREFIX})
    pkg_check_modules(mariadbclient_PKGCONF QUIET "lib${mariadbclient_LIBNAME}")
endif()

if(NOT mariadbclient_PKGCONF_FOUND)
    message(FATAL_ERROR "pkg-config cannot find ${mariadbclient_LIBNAME}")
endif()

# Set include directory
find_path(MariaDBClient_INCLUDE_DIR mysql.h
        HINTS ${mariadbclient_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES mariadb
        )

# Handle static libraries
if(MariaDBClient_USE_STATIC_LIBS)
    # Save current value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(mariadbclient_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(MariaDBClient_LIBRARY
        NAMES ${mariadbclient_LIBNAME}
        HINTS ${mariadbclient_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )
if (MariaDBClient_LIBRARY)
    # NOTE: This must be set for find_package_handle_standard_args to work
    set(MariaDBClient_FOUND ON)
endif()

if(MariaDBClient_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${mariadbclient_LIBNAME} mariadbclient
                                  "${mariadbclient_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${mariadbclient_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(mariadbclient_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

FindDynamicLibraryDependencies(mariadbclient "${mariadbclient_DYNAMIC_LIBS}")

# Set version
set(MariaDBClient_VERSION ${mariadbclient_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MariaDBClient
        REQUIRED_VARS MariaDBClient_INCLUDE_DIR
        VERSION_VAR MariaDBClient_VERSION
        )

if(NOT TARGET MariaDBClient::MariaDBClient)
    # Add library to build
    if (MariaDBClient_FOUND)
        if (MariaDBClient_USE_STATIC_LIBS)
            add_library(MariaDBClient::MariaDBClient STATIC IMPORTED)
        else()
            # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
            # libraries installed, we can still use the STATIC libraries
            add_library(MariaDBClient::MariaDBClient UNKNOWN IMPORTED)
        endif()
    endif()

    # Set include directories for library
    if(MariaDBClient_INCLUDE_DIR)
        set_target_properties(MariaDBClient::MariaDBClient
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${MariaDBClient_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${MariaDBClient_LIBRARY}")
        set_target_properties(MariaDBClient::MariaDBClient
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${MariaDBClient_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(mariadbclient_LIBRARY_DEPENDENCIES)
            set_target_properties(MariaDBClient::MariaDBClient
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${mariadbclient_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

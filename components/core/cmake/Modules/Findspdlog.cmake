# Try to find spdlog
#
# Set spdlog_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done this will define:
#  spdlog_FOUND - Whether spdlog was found on the system
#  spdlog_INCLUDE_DIR - The spdlog include directories
#  spdlog_VERSION - The version of spdlog installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "SPDLOG_"
# - Variables that should be externally visible are prefixed with "spdlog_"

set(SPDLOG_LIBNAME "spdlog")
set(SPDLOG_TARGET_NAME "spdlog::spdlog")

include(cmake/Modules/FindLibraryDependencies.cmake)

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(SPDLOG_PKGCONF QUIET ${SPDLOG_LIBNAME})

# Set include directory
find_path(
    spdlog_INCLUDE_DIR
    spdlog.h
    HINTS ${SPDLOG_PKGCONF_INCLUDEDIR}
    PATH_SUFFIXES spdlog
)

# Handle static libraries
if(spdlog_USE_STATIC_LIBS)
    # Save current value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(SPDLOG_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(
    SPDLOG_LIBRARY
    NAMES ${SPDLOG_LIBNAME}
    HINTS ${SPDLOG_PKGCONF_LIBDIR}
    PATH_SUFFIXES lib
)
if (SPDLOG_LIBRARY)
    # NOTE: This must be set for find_package_handle_standard_args to work
    set(spdlog_FOUND ON)
endif()

if(spdlog_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${SPDLOG_LIBNAME} SPDLOG "${SPDLOG_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${SPDLOG_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(SPDLOG_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

FindDynamicLibraryDependencies(SPDLOG "${SPDLOG_PKGCONF_LIBRARIES}")

# Set version
set(spdlog_VERSION ${SPDLOG_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ${SPDLOG_LIBNAME}
    REQUIRED_VARS spdlog_INCLUDE_DIR
    VERSION_VAR spdlog_VERSION
)

if(NOT TARGET ${SPDLOG_TARGET_NAME})
    # Add library to build
    if (spdlog_FOUND)
        if (spdlog_USE_STATIC_LIBS)
            add_library(${SPDLOG_TARGET_NAME} STATIC IMPORTED)
            set_target_properties(
                ${SPDLOG_TARGET_NAME}
                PROPERTIES
                COMPILE_FLAGS "${SPDLOG_PKGCONF_STATIC_CFLAGS}"
            )
        else()
            # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
            # libraries installed, we can still use the STATIC libraries.
            add_library(${SPDLOG_TARGET_NAME} UNKNOWN IMPORTED)
            set_target_properties(
                ${SPDLOG_TARGET_NAME}
                PROPERTIES
                COMPILE_FLAGS "${SPDLOG_PKGCONF_CFLAGS}"
            )
        endif()
    endif()

    # Set include directories for library
    if (NOT EXISTS "${spdlog_INCLUDE_DIR}")
        set(spdlog_FOUND OFF)
    else()
        set_target_properties(
            ${SPDLOG_TARGET_NAME}
            PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${spdlog_INCLUDE_DIR}"
        )
    endif()

    # Set location of library
    if(NOT EXISTS "${SPDLOG_LIBRARY}")
        set(spdlog_FOUND OFF)
    else()
        set_target_properties(
                ${SPDLOG_TARGET_NAME}
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${SPDLOG_LIBRARY}"
        )

        # Add component's dependencies for linking
        if(SPDLOG_LIBRARY_DEPENDENCIES)
            set_target_properties(
                    ${SPDLOG_TARGET_NAME}
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${SPDLOG_LIBRARY_DEPENDENCIES}"
            )
        endif()
    endif()
endif()

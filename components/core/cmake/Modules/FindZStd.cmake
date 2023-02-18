# Try to find ZStd
#
# Set ZStd_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done this will define:
#  ZStd_FOUND - Whether ZStd was found on the system
#  ZStd_INCLUDE_DIR - The ZStd include directories
#  ZStd_VERSION - The version of ZStd installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "zstd_"
# - Variables that should be externally visible are prefixed with "ZStd_"

set(zstd_LIBNAME "zstd")

include(cmake/Modules/FindLibraryDependencies.cmake)

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(zstd_PKGCONF QUIET lib${zstd_LIBNAME})

# Set include directory
find_path(ZStd_INCLUDE_DIR zstd.h
        HINTS ${zstd_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(ZStd_USE_STATIC_LIBS)
    # Save current value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(zstd_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(ZStd_LIBRARY
        NAMES zstd
        HINTS ${zstd_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )
if (ZStd_LIBRARY)
    # NOTE: This must be set for find_package_handle_standard_args to work
    set(ZStd_FOUND ON)
endif()

if(ZStd_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${zstd_LIBNAME} zstd "${zstd_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${zstd_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(zstd_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# Add pthread manually since zstd's pkgconfig doesn't include it
list(APPEND zstd_DYNAMIC_LIBS "pthread")

FindDynamicLibraryDependencies(zstd "${zstd_DYNAMIC_LIBS}")

# Set version
set(ZStd_VERSION ${zstd_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZStd
        REQUIRED_VARS ZStd_INCLUDE_DIR
        VERSION_VAR ZStd_VERSION
        )

if(NOT TARGET ZStd::ZStd)
    # Add library to build
    if (ZStd_FOUND)
        if (ZStd_USE_STATIC_LIBS)
            add_library(ZStd::ZStd STATIC IMPORTED)
        else()
            # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
            # libraries installed, we can still use the STATIC libraries
            add_library(ZStd::ZStd UNKNOWN IMPORTED)
        endif()
    endif()

    # Set include directories for library
    if(ZStd_INCLUDE_DIR)
        set_target_properties(ZStd::ZStd
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ZStd_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${ZStd_LIBRARY}")
        set_target_properties(ZStd::ZStd
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${ZStd_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(zstd_LIBRARY_DEPENDENCIES)
            set_target_properties(ZStd::ZStd
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${zstd_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

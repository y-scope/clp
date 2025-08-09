# Try to find BZip2
# NOTE: The FindBZip2.cmake included with CMake has no support for static libraries, so we use our
# own.
#
# Set BZip2_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done, this will define:
#  BZip2_FOUND - Whether the library was found on the system
#  BZip2_INCLUDE_DIR - The library include directories
#  BZip2_LIBRARY - The path to the library file
#
# And will define the following if applicable:
#  BZip2_VERSION - The version of the library installed on the system
#  BZip2_LIBRARY_DEPENDENCIES - Any additional modules required to link with the library
#
# Conventions:
# - Variables only for use within the script are prefixed with "bzip2_"
# - Variables that should be externally visible are prefixed with "BZip2_"

include(cmake/Modules/FindLibraryDependencies.cmake)

set(bzip2_HEADER "bzlib.h")
set(bzip2_LIBNAME "bz2")
set(bzip2_LOCAL_PREFIX "bzip2")
set(bzip2_PKGCONFIG_NAME "bzip2")

if(DEFINED BZip2_ROOT)
    set(bzip2_PKGCONFIG_DIR "${BZip2_ROOT}/lib/pkgconfig")
    set(ENV{bzip2_ORIG_PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${bzip2_PKGCONFIG_DIR};$ENV{PKG_CONFIG_PATH}")
endif()

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(bzip2_PKGCONF QUIET "${bzip2_PKGCONFIG_NAME}")

# Manually set package finding hints in case of failure
if(NOT bzip2_PKGCONF_FOUND)
    message(WARNING "Cannot find package config for ${bzip2_PKGCONFIG_NAME}")
    if(DEFINED BZip2_ROOT)
        set(bzip2_PKGCONF_INCLUDEDIR "${BZip2_ROOT}/include")
        set(bzip2_PKGCONF_LIBDIR "${BZip2_ROOT}/lib")
    endif()
endif()

# Set include directory
find_path(BZip2_INCLUDE_DIR ${bzip2_HEADER}
        HINTS ${bzip2_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(BZip2_USE_STATIC_LIBS)
    set(bzip2_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(BZip2_LIBRARY
        NAMES "${bzip2_LIBNAME}"
        HINTS ${bzip2_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )

if(BZip2_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${bzip2_LIBNAME} ${bzip2_LOCAL_PREFIX}
                                  "${bzip2_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${bzip2_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(bzip2_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

FindDynamicLibraryDependencies(${bzip2_LOCAL_PREFIX} "${bzip2_DYNAMIC_LIBS}")

# Set version
set(BZip2_VERSION ${bzip2_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BZip2
        REQUIRED_VARS BZip2_LIBRARY BZip2_INCLUDE_DIR
        VERSION_VAR BZip2_VERSION
        )

if(NOT TARGET BZip2::BZip2)
    # Add library to build
    if (BZip2_USE_STATIC_LIBS)
        add_library(BZip2::BZip2 STATIC IMPORTED)
    else()
        # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
        # libraries installed, we can still use the STATIC libraries
        add_library(BZip2::BZip2 UNKNOWN IMPORTED)
    endif()

    # Set include directories for library
    if(BZip2_INCLUDE_DIR)
        set_target_properties(BZip2::BZip2
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${BZip2_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${BZip2_LIBRARY}")
        set_target_properties(BZip2::BZip2
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${BZip2_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(bzip2_LIBRARY_DEPENDENCIES)
            set_target_properties(BZip2::BZip2
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${bzip2_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

# Restore original value of PKG_CONFIG_PATH
if(DEFINED BZip2_ROOT)
    set(ENV{PKG_CONFIG_PATH} "$ENV{bzip2_ORIG_PKG_CONFIG_PATH}")
    unset(ENV{bzip2_ORIG_PKG_CONFIG_PATH})
endif()

# Try to find LZ4
#
# Set LZ4_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done this will define:
#  LZ4_FOUND - Whether LZ4 was found on the system
#  LZ4_INCLUDE_DIR - The LZ4 include directories
#  LZ4_VERSION - The version of LZ4 installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "lz4_"
# - Variables that should be externally visible are prefixed with "LZ4_"

set(lz4_LIBNAME "lz4")

include(cmake/Modules/FindLibraryDependencies.cmake)

set(lz4_HEADER "lz4.h")
set(lz4_LIBNAME "lz4")
set(lz4_LOCAL_PREFIX "lz4")
set(lz4_PKGCONFIG_NAME "liblz4")

if(DEFINED LZ4_ROOT)
    set(lz4_PKGCONFIG_DIR "${LZ4_ROOT}/lib/pkgconfig")
    set(ENV{lz4_ORIG_PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${lz4_PKGCONFIG_DIR};$ENV{PKG_CONFIG_PATH}")
endif()

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(lz4_PKGCONF QUIET "${lz4_PKGCONFIG_NAME}")

# Manually set package finding hints in case of failure
if(NOT lz4_PKGCONF_FOUND)
    message(WARNING "Cannot find package config for ${lz4_PKGCONFIG_NAME}")
    if(DEFINED LZ4_ROOT)
        set(lz4_PKGCONF_INCLUDEDIR "${LZ4_ROOT}/include")
        set(lz4_PKGCONF_LIBDIR "${LZ4_ROOT}/lib")
    endif()
endif()

# Set include directory
find_path(LZ4_INCLUDE_DIR ${lz4_HEADER}
        HINTS ${lz4_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(LZ4_USE_STATIC_LIBS)
    set(lz4_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(LZ4_LIBRARY
        NAMES ${lz4_LIBNAME}
        HINTS ${lz4_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )

if(LZ4_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${lz4_LIBNAME} ${lz4_LOCAL_PREFIX}
                                  "${lz4_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${lz4_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(lz4_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

FindDynamicLibraryDependencies(${lz4_LOCAL_PREFIX} "${lz4_DYNAMIC_LIBS}")

# Set version
set(LZ4_VERSION ${lz4_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4
        REQUIRED_VARS LZ4_LIBRARY LZ4_INCLUDE_DIR
        VERSION_VAR LZ4_VERSION
        )

if(NOT TARGET LZ4::LZ4)
    # Add library to build
    if (LZ4_USE_STATIC_LIBS)
        add_library(LZ4::LZ4 STATIC IMPORTED)
    else()
        # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
        # libraries installed, we can still use the STATIC libraries
        add_library(LZ4::LZ4 UNKNOWN IMPORTED)
    endif()

    # Set include directories for library
    if(LZ4_INCLUDE_DIR)
        set_target_properties(LZ4::LZ4
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${LZ4_LIBRARY}")
        set_target_properties(LZ4::LZ4
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LZ4_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(lz4_LIBRARY_DEPENDENCIES)
            set_target_properties(LZ4::LZ4
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${lz4_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

# Restore original value of PKG_CONFIG_PATH
if(DEFINED LZ4_ROOT)
    set(ENV{PKG_CONFIG_PATH} "$ENV{lz4_ORIG_PKG_CONFIG_PATH}")
    unset(ENV{lz4_ORIG_PKG_CONFIG_PATH})
endif()

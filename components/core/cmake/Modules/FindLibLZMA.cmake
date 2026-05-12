# Try to find LibLZMA
# NOTE: The FindLibLZMA.cmake included with CMake has no support for static libraries, so we use our
# own.
#
# Set LibLZMA_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done, this will define:
#  LibLZMA_FOUND - Whether the library was found on the system
#  LibLZMA_INCLUDE_DIR - The library include directories
#  LibLZMA_LIBRARY - The path to the library file
#  LibLZMA_VERSION - The version of the library installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "liblzma_"
# - Variables that should be externally visible are prefixed with "LibLZMA_"

set(liblzma_HEADER "lzma.h")
set(liblzma_LIBNAME "lzma")
set(liblzma_PKGCONFIG_NAME "liblzma")

if(DEFINED CLP_DEPS_CPP_DIR)
    set(ENV{liblzma_ORIG_PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${CLP_DEPS_CPP_DIR}:$ENV{PKG_CONFIG_PATH}")
endif()

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(liblzma_PKGCONF QUIET "${liblzma_PKGCONFIG_NAME}")

# Set include directory
find_path(LibLZMA_INCLUDE_DIR ${liblzma_HEADER}
        HINTS ${liblzma_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(LibLZMA_USE_STATIC_LIBS)
    set(liblzma_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

# Find library
find_library(LibLZMA_LIBRARY
        NAMES "${liblzma_LIBNAME}"
        HINTS ${liblzma_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )

if(LibLZMA_USE_STATIC_LIBS)
    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${liblzma_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(liblzma_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# Set version
set(LibLZMA_VERSION ${liblzma_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibLZMA
        REQUIRED_VARS LibLZMA_LIBRARY LibLZMA_INCLUDE_DIR
        VERSION_VAR LibLZMA_VERSION
        )

if(NOT TARGET LibLZMA::LibLZMA)
    # Add library to build
    if (LibLZMA_USE_STATIC_LIBS)
        add_library(LibLZMA::LibLZMA STATIC IMPORTED)
    else()
        # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
        # libraries installed, we can still use the STATIC libraries
        add_library(LibLZMA::LibLZMA UNKNOWN IMPORTED)
    endif()

    # Set include directories for library
    if(LibLZMA_INCLUDE_DIR)
        set_target_properties(LibLZMA::LibLZMA
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LibLZMA_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${LibLZMA_LIBRARY}")
        set_target_properties(LibLZMA::LibLZMA
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LibLZMA_LIBRARY}"
                )
    endif()
endif()

# Restore original value of PKG_CONFIG_PATH
if(DEFINED CLP_DEPS_CPP_DIR)
    set(ENV{PKG_CONFIG_PATH} "$ENV{liblzma_ORIG_PKG_CONFIG_PATH}")
    unset(ENV{liblzma_ORIG_PKG_CONFIG_PATH})
endif()

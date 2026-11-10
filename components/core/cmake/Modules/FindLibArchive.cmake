# Try to find LibArchive
# NOTE: The FindLibArchive.cmake included with CMake has no support for static libraries, so we use
# our own.
#
# Set LibArchive_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done, this will define:
#  LibArchive_FOUND - Whether the library was found on the system
#  LibArchive_INCLUDE_DIR - The library include directories
#  LibArchive_LIBRARY - The path to the library file
#
# And will define the following if the package configuration file provides relevant information:
#  LibArchive_VERSION - The version of library installed on the system
#  LibArchive_LIBRARY_DEPENDENCIES - Any additional modules required to link with the library
#
# Conventions:
# - Variables only for use within the script are prefixed with "libarchive_"
# - Variables that should be externally visible are prefixed with "LibArchive_"

include(FindPackageHandleStandardArgs)

include(cmake/Modules/FindLibraryDependencies.cmake)

set(libarchive_HEADER "archive.h")
set(libarchive_LIBNAME "archive")
set(libarchive_LOCAL_PREFIX "libarchive")
set(libarchive_PKGCONFIG_NAME "libarchive")

if(DEFINED LibArchive_ROOT)
    set(libarchive_PKGCONFIG_DIR "${LibArchive_ROOT}/lib/pkgconfig")
    set(ENV{libarchive_ORIG_PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}")
    set(ENV{PKG_CONFIG_PATH} "${libarchive_PKGCONFIG_DIR};$ENV{PKG_CONFIG_PATH}")
endif()

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(libarchive_PKGCONF QUIET "${libarchive_PKGCONFIG_NAME}")

# Set include directory
find_path(LibArchive_INCLUDE_DIR ${libarchive_HEADER}
        HINTS ${libarchive_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(LibArchive_USE_STATIC_LIBS)
    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(LibArchive_LIBRARY
        NAMES ${libarchive_LIBNAME}
        HINTS ${libarchive_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )

# Find dependencies
if(LibArchive_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${libarchive_LIBNAME} ${libarchive_LOCAL_PREFIX}
                                  "${libarchive_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

FindDynamicLibraryDependencies(libarchive "${libarchive_DYNAMIC_LIBS}")

message(STATUS "libarchive_PKGCONF_STATIC_LIBRARIES = ${libarchive_PKGCONF_STATIC_LIBRARIES}")
message(STATUS "libarchive_DYNAMIC_LIBS = ${libarchive_DYNAMIC_LIBS}")
message(STATUS "libarchive_LIBRARY_DEPENDENCIES = ${libarchive_LIBRARY_DEPENDENCIES}")

# Set version
set(LibArchive_VERSION ${libarchive_PKGCONF_VERSION})

# Set up find_package() call
find_package_handle_standard_args(LibArchive
        REQUIRED_VARS LibArchive_LIBRARY LibArchive_INCLUDE_DIR
        VERSION_VAR LibArchive_VERSION
        )

if(NOT TARGET LibArchive::LibArchive)
    # Add library to build
    if(LibArchive_USE_STATIC_LIBS)
        add_library(LibArchive::LibArchive STATIC IMPORTED)
    else()
        # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED
        # libraries installed, we can still use the STATIC libraries
        add_library(LibArchive::LibArchive UNKNOWN IMPORTED)
    endif()

    # Set include directories for library
    if(LibArchive_INCLUDE_DIR)
        set_target_properties(LibArchive::LibArchive
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LibArchive_INCLUDE_DIR}"
                )
    endif()

    # Set location of library
    if(EXISTS "${LibArchive_LIBRARY}")
        set_target_properties(LibArchive::LibArchive
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LibArchive_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(libarchive_LIBRARY_DEPENDENCIES)
            set_target_properties(LibArchive::LibArchive
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${libarchive_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

# Restore original value of PKG_CONFIG_PATH
if(DEFINED ENV{libarchive_ORIG_PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_PATH} "$ENV{libarchive_ORIG_PKG_CONFIG_PATH}")
    unset(ENV{libarchive_ORIG_PKG_CONFIG_PATH})
endif()

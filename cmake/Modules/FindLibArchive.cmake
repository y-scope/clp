# Try to find LibArchive
# NOTE: The FindLibArchive.cmake included with CMake has no support for static libraries, so we use our own.
#
# Set LibArchive_USE_STATIC_LIBS=ON to look for static libraries.
#
# Once done this will define:
#  LibArchive_FOUND - Whether LibArchive was found on the system
#  LibArchive_INCLUDE_DIR - The LibArchive include directories
#  LibArchive_VERSION - The version of LibArchive installed on the system
#
# Conventions:
# - Variables only for use within the script are prefixed with "libarchive_"
# - Variables that should be externally visible are prefixed with "LibArchive_"

set(libarchive_LIBNAME "archive")

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(libarchive_PKGCONF QUIET "lib${libarchive_LIBNAME}")

# Set include directory
find_path(LibArchive_INCLUDE_DIR archive.h
        HINTS ${libarchive_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES include
        )

# Handle static libraries
if(LibArchive_USE_STATIC_LIBS)
    # Save current value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(LibArchive_LIBRARY
        NAMES ${libarchive_LIBNAME}
        HINTS ${libarchive_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )
if (LibArchive_LIBRARY)
    # NOTE: This must be set for find_package_handle_standard_args to work
    set(LibArchive_FOUND ON)
endif()

if(LibArchive_USE_STATIC_LIBS)
    # NOTE: libc, libm, libpthread, and librt should be dynamically linked
    set(libarchive_UNLINKABLE_LIBS "c;m;pthread;rt;${libarchive_LIBNAME}")

    # Get absolute path of dependent libraries
    foreach(libarchive_LIBNAME ${libarchive_PKGCONF_STATIC_LIBRARIES})
        # Skip unlinkable libs
        set(libarchive_IS_UNLINKABLE_LIB FALSE)
        foreach(libarchive_UNLINKABLE_LIBNAME ${libarchive_UNLINKABLE_LIBS})
            if (${libarchive_LIBNAME} STREQUAL ${libarchive_UNLINKABLE_LIBNAME})
                set(libarchive_IS_UNLINKABLE_LIB TRUE)
            endif()
        endforeach()
        if(libarchive_IS_UNLINKABLE_LIB)
            continue()
        endif()

        find_library(libarchive_${libarchive_LIBNAME}_LIBRARY
                NAMES ${libarchive_LIBNAME}
                PATH_SUFFIXES lib
                )
        if(libarchive_${libarchive_LIBNAME}_LIBRARY)
            list(APPEND libarchive_EXTERNAL_DEPENDENCIES "${libarchive_${libarchive_LIBNAME}_LIBRARY}")
        else()
            message(SEND_ERROR "Static ${libarchive_LIBNAME} library not found")
        endif()
    endforeach()

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(libarchive_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# Set version
set(LibArchive_VERSION ${libarchive_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibArchive
        REQUIRED_VARS LibArchive_INCLUDE_DIR
        VERSION_VAR LibArchive_VERSION
        )

if(NOT TARGET LibArchive::LibArchive)
    # Add library to build
    if (LibArchive_FOUND)
        if (LibArchive_USE_STATIC_LIBS)
            add_library(LibArchive::LibArchive STATIC IMPORTED)
        else()
            # NOTE: We use UNKNOWN so that if the user doesn't have the SHARED libraries installed, we can still use the STATIC libraries
            add_library(LibArchive::LibArchive UNKNOWN IMPORTED)
        endif()
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
        if(libarchive_EXTERNAL_DEPENDENCIES)
            set_target_properties(LibArchive::LibArchive
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${libarchive_EXTERNAL_DEPENDENCIES}"
                    )
        endif()
    endif()
endif()

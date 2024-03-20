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
# - Variables only for use within the script are prefixed with "spdlog_"
# - Variables that should be externally visible are prefixed with "spdlog_"

set(spdlog_LIBNAME "spdlog")

include(cmake/Modules/FindLibraryDependencies.cmake)

# Run pkg-config
find_package(PkgConfig)
pkg_check_modules(spdlog_PKGCONF QUIET spdlog)

# Set include directory
find_path(spdlog_INCLUDE_DIR spdlog.h
        HINTS ${spdlog_PKGCONF_INCLUDEDIR}
        PATH_SUFFIXES spdlog
        )

# Handle static libraries
if(spdlog_USE_STATIC_LIBS)
    # Save current value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(spdlog_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Temporarily change CMAKE_FIND_LIBRARY_SUFFIXES to static library suffix
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Find library
find_library(spdlog_LIBRARY
        NAMES spdlog
        HINTS ${spdlog_PKGCONF_LIBDIR}
        PATH_SUFFIXES lib
        )
if (spdlog_LIBRARY)
    # NOTE: This must be set for find_package_handle_standard_args to work
    set(spdlog_FOUND ON)
endif()

if(spdlog_USE_STATIC_LIBS)
    FindStaticLibraryDependencies(${spdlog_LIBNAME} spdlog "${spdlog_PKGCONF_STATIC_LIBRARIES}")

    # Restore original value of CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${spdlog_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(spdlog_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
else()
    # Add dependencies manually since spdlog's pkgconfig doesn't include it
    list(APPEND spdlog_DYNAMIC_LIBS "fmt")
endif()

list(APPEND spdlog_DYNAMIC_LIBS "pthread")
FindDynamicLibraryDependencies(spdlog "${spdlog_DYNAMIC_LIBS}")

# Set version
set(spdlog_VERSION ${spdlog_PKGCONF_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
        REQUIRED_VARS spdlog_INCLUDE_DIR
        VERSION_VAR spdlog_VERSION
        )

if(NOT TARGET spdlog::spdlog)
    # Add library to build
    if (spdlog_FOUND)
        if (spdlog_USE_STATIC_LIBS)
            add_library(spdlog::spdlog STATIC IMPORTED)
        else()
            # NOTE: libspdlog is only available as a static library or a dynamic one.
            add_library(spdlog::spdlog SHARED IMPORTED)
        endif()
    endif()

    # Set include directories for library
    if(EXISTS "${spdlog_INCLUDE_DIR}")
        set_target_properties(spdlog::spdlog
                PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${spdlog_INCLUDE_DIR}"
                )
    else()
        set(spdlog_FOUND OFF)
    endif()

    # Set location of library
    if(EXISTS "${spdlog_LIBRARY}")
        set_target_properties(spdlog::spdlog
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${spdlog_LIBRARY}"
                )

        # Add component's dependencies for linking
        if(spdlog_LIBRARY_DEPENDENCIES)
            set_target_properties(spdlog::spdlog
                    PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${spdlog_LIBRARY_DEPENDENCIES}"
                    )
        endif()
    else()
        set(spdlog_FOUND OFF)
    endif()
endif()

# Find the given libraries ignoring libraries that should be dynamically linked
# Params:
#   fld_LIBNAME - Name of the library (without the 'lib' prefix)
#   fld_PREFIX - Prefix for created variables
#   fld_STATIC_LIBS - List of libraries to find
# Returns:
#   ${fld_LIBNAME}_DYNAMIC_LIBS - List of libraries that should be dynamically linked
#   ${fld_PREFIX}_LIBRARY_DEPENDENCIES - Found libraries
macro(FindStaticLibraryDependencies fld_LIBNAME fld_PREFIX fld_STATIC_LIBS)
    # NOTE: libc, libm, libpthread, and librt should be dynamically linked
    set(fld_DYNAMIC_LIBS "c;m;pthread;rt")

    # Get absolute path of dependent libraries
    foreach(fld_DEP_LIB ${fld_STATIC_LIBS})
        # Skip dynamic libs
        set(fld_IGNORE_LIB FALSE)
        foreach(fld_DYNAMIC_LIB ${fld_DYNAMIC_LIBS})
            if(${fld_DEP_LIB} STREQUAL ${fld_DYNAMIC_LIB})
                set(fld_IGNORE_LIB TRUE)
                list(APPEND ${fld_PREFIX}_DYNAMIC_LIBS "${fld_DEP_LIB}")
            endif()
        endforeach()
        if(fld_IGNORE_LIB OR ${fld_DEP_LIB} STREQUAL ${fld_LIBNAME})
            continue()
        endif()

        find_library(${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY
                NAMES ${fld_DEP_LIB}
                PATH_SUFFIXES lib
                )
        if(${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY)
            list(APPEND ${fld_PREFIX}_LIBRARY_DEPENDENCIES
                 "${${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY}")
        else()
            message(SEND_ERROR "Static ${fld_DEP_LIB} library not found")
        endif()
    endforeach()
endmacro()

# Find the given libraries
# Params:
#   fld_PREFIX - Prefix for created variables
#   fld_DYNAMIC_LIBS - List of libraries to find
# Returns:
#   ${fld_PREFIX}_LIBRARY_DEPENDENCIES - Found libraries
macro(FindDynamicLibraryDependencies fld_PREFIX fld_DYNAMIC_LIBS)
    foreach(fld_DEP_LIB ${fld_DYNAMIC_LIBS})
        find_library(${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY
                NAMES ${fld_DEP_LIB}
                PATH_SUFFIXES lib
                )
        if(${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY)
            list(APPEND ${fld_PREFIX}_LIBRARY_DEPENDENCIES "${${fld_PREFIX}_${fld_DEP_LIB}_LIBRARY}")
        else()
            message(SEND_ERROR "${fld_DEP_LIB} library not found")
        endif()
    endforeach()
endmacro()

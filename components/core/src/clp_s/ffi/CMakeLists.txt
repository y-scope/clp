set(
        CLP_S_FFI_SFA_SOURCES
        sfa/ClpArchiveReader.cpp
        sfa/ClpArchiveReader.hpp
)

if(CLP_BUILD_CLP_S_ARCHIVEREADER)
        add_library(
                clp_s_ffi_sfa
                ${CLP_S_FFI_SFA_SOURCES}
        )
        add_library(clp_s::ffi::sfa ALIAS clp_s_ffi_sfa)
        target_compile_features(clp_s_ffi_sfa PRIVATE cxx_std_20)
        target_include_directories(clp_s_ffi_sfa PUBLIC ../../)
        target_link_libraries(
                clp_s_ffi_sfa
                PUBLIC
                clp_s::archive_reader
        )
endif()

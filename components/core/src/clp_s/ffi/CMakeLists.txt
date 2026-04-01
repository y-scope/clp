set(
        CLP_S_FFI_SFA_SOURCES
        sfa/ClpArchiveDecoder.cpp
        sfa/ClpArchiveDecoder.hpp
        sfa/SfaErrorCode.cpp
        sfa/SfaErrorCode.hpp
        sfa/ClpArchiveReader.cpp
        sfa/ClpArchiveReader.hpp
        sfa/LogEvent.hpp
)

if(CLP_BUILD_CLP_S_ARCHIVEREADER)
        add_library(
                clp_s_ffi_sfa
                ${CLP_S_FFI_SFA_SOURCES}
        )
        add_library(clp_s::ffi::sfa ALIAS clp_s_ffi_sfa)
        target_compile_features(clp_s_ffi_sfa PRIVATE cxx_std_20)
        target_include_directories(clp_s_ffi_sfa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
        target_link_libraries(
                clp_s_ffi_sfa
                PUBLIC
                clp_s::archive_reader
                ystdlib::error_handling
                PRIVATE
                spdlog::spdlog
        )
endif()

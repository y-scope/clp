set(
        IO_CORE_HEADER_LIST
        "ReaderInterface.hpp"
        "WriterInterface.hpp"
)
set(
        IO_CORE_SOURCE_LIST
        "ReaderInterface.cpp"
        "WriterInterface.cpp"
)
add_library(
        io_interface
        ${IO_CORE_HEADER_LIST}
        ${IO_CORE_SOURCE_LIST}
)
add_library(clp::io_interface ALIAS io_interface)
target_include_directories(io_interface PUBLIC ../)
target_compile_features(io_interface PRIVATE cxx_std_20)

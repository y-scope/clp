#ifndef CLP_FFI_IR_STREAM_BYTESWAP_HPP
#define CLP_FFI_IR_STREAM_BYTESWAP_HPP

#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>

    // TODO: Improve byte swapping implementation by replacing macros with a templated function.
    // Ideally, we should implement a general, type-safe, templated byte swap function where the
    // input size is inferred from the type. This would:
    // - Improve usability by removing the need to specify the size explicitly.
    // - Encapsulate platform-specific details, improving portability and maintainability.

    // NOTE: Currently, we disable the following two checks for the following reasons:
    // - We're mirroring the GNU `byteswap.h` method names (e.g., `bswap_16`), so we're using
    //   lowercase names.
    // - We're using macros as wrappers around platform-specific byte swap functions because we
    //   don't yet have a proper templated implementation.
    // NOLINTBEGIN(cppcoreguidelines-macro-usage, readability-identifier-naming)
    #define bswap_16(x) OSSwapInt16(x)
    #define bswap_32(x) OSSwapInt32(x)
    #define bswap_64(x) OSSwapInt64(x)
    // NOLINTEND(cppcoreguidelines-macro-usage, readability-identifier-naming)
#elif defined(_MSC_VER)
    #include <stdlib.h>

    // NOTE: The following two checks are disabled for the same reasons as metioned above.
    // NOLINTBEGIN(cppcoreguidelines-macro-usage, readability-identifier-naming)
    #define bswap_16(x) _byteswap_ushort(x)
    #define bswap_32(x) _byteswap_ulong(x)
    #define bswap_64(x) _byteswap_uint64(x)
    // NOLINTEND(cppcoreguidelines-macro-usage, readability-identifier-naming)
#else
    // IWYU pragma: begin_exports
    #include <byteswap.h>
    // IWYU pragma: end_exports
#endif

#endif  // CLP_FFI_IR_STREAM_BYTESWAP_HPP

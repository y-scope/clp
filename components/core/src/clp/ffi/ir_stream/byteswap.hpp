#ifndef CLP_FFI_IR_STREAM_BYTESWAP_HPP
#define CLP_FFI_IR_STREAM_BYTESWAP_HPP

#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>

    // TODO: Improve byte swapping implementation by replacing macros with a templated function.
    // Ideally, we should implement a general, type-safe, templated byte swap function where the
    // input size is inferred from the type. This would:
    // - Improve usability by removing the need to specify the size explicitly.
    // - Encapsulate platform-specific details, improving portability and maintainability.

    // NOTE:
    // - We disable `cppcoreguidelines-macro-usage` since we're using macros as wrappers around
    //   platform-specific byte swap functions, and we don't yet have a proper templated
    //   implementation.
    // - We disable `readability-identifier-naming` since we're mirroring the GNU `byteswap.h`
    //   method names (e.g., `bswap_16`) which are lowercase.
    // NOLINTBEGIN(cppcoreguidelines-macro-usage, readability-identifier-naming)
    #define bswap_16(x) OSSwapInt16(x)
    #define bswap_32(x) OSSwapInt32(x)
    #define bswap_64(x) OSSwapInt64(x)
    // NOLINTEND(cppcoreguidelines-macro-usage, readability-identifier-naming)
#elif defined(_MSC_VER)
    #include <stdlib.h>

    // We disable the checks below for the same reasons mentioned above.
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

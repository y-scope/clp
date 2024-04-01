#ifndef CLP_FFI_IR_STREAM_BYTESWAP_HPP
#define CLP_FFI_IR_STREAM_BYTESWAP_HPP

#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define bswap_16(x) OSSwapInt16(x)
    #define bswap_32(x) OSSwapInt32(x)
    #define bswap_64(x) OSSwapInt64(x)
#elif defined(_MSC_VER)
    #include <stdlib.h>
    #define bswap_16(x) _byteswap_ushort(x)
    #define bswap_32(x) _byteswap_ulong(x)
    #define bswap_64(x) _byteswap_uint64(x)
#else
    #include <byteswap.h>
#endif

#endif  // CLP_FFI_IR_STREAM_BYTESWAP_HPP

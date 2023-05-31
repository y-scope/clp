
#ifndef IrMessageParser_HPP
#define IrMessageParser_HPP

// C standard libraries

// C++ standard libraries

// Project headers
#include "TraceableException.hpp"
#include "ffi/ir_stream/decoding_methods.hpp"
#include "ParsedIrMessage.hpp"

class IrMessageParser {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
            TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "IrMessageParser operation failed";
        }
    };

    // Methods
    static bool parse_four_bytes_encoded_message(ReaderInterface& reader,
                                                 ParsedIrMessage& msg,
                                                 epochtime_t& reference_ts);
};

#endif // IrMessageParser_HPP

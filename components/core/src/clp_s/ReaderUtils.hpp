#ifndef CLP_S_READERUTILS_HPP
#define CLP_S_READERUTILS_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <system_error>

#include <ystdlib/error_handling/Result.hpp>

#include "ArchiveReaderAdaptor.hpp"
#include "ErrorCode.hpp"
#include "Schema.hpp"
#include "SchemaTree.hpp"
#include "TraceableException.hpp"

namespace clp_s {
class ReaderUtils {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    using SchemaMap = std::map<int32_t, Schema>;
    static constexpr size_t cDecompressorFileReadBufferCapacity{64ULL * 1024ULL};  // 64 KiB

    /**
     * Reads the schema tree from an archive
     * @param adaptor
     * @return The schema tree
     */
    static auto read_schema_tree(ArchiveReaderAdaptor& adaptor) -> std::shared_ptr<SchemaTree>;

    /**
     * Reads the schema map from an archive
     * @param adaptor
     * @return the schema map
     */
    static auto read_schemas(ArchiveReaderAdaptor& adaptor) -> std::shared_ptr<SchemaMap>;

    /**
     * Reads the parsing specification from an archive.
     * @param adaptor
     * @return The parsing specification contents.
     * @throw OperationFailed if the schema is missing or corrupt.
     */
    static auto read_parsing_spec(ArchiveReaderAdaptor& adaptor) -> std::string;

    /**
     * Converts a serialized 64-bit numeric value into `size_t` with bounds checking.
     * @param value The 64-bit value deserialized from archive metadata.
     * @return Converted `size_t` on success.
     * @return A result containing the converted value on success, or an error code indicating the
     * failure:
     * - std::errc::value_too_large if the value cannot fit in `size_t`.
     */
    [[nodiscard]] static auto try_uint64_to_size_t(uint64_t value)
            -> ystdlib::error_handling::Result<size_t> {
        if (value > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return std::errc::value_too_large;
        }
        return static_cast<size_t>(value);
    }
};
}  // namespace clp_s

#endif  // CLP_S_READERUTILS_HPP

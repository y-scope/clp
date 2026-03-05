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
#include "DictionaryReader.hpp"
#include "Schema.hpp"
#include "SchemaReader.hpp"
#include "SchemaTree.hpp"
#include "TimestampDictionaryReader.hpp"
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
    static constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KiB

    /**
     * Reads the schema tree from an archive
     * @param adaptor
     * @return The schema tree
     */
    static std::shared_ptr<SchemaTree> read_schema_tree(ArchiveReaderAdaptor& adaptor);

    /**
     * Reads the schema map from an archive
     * @param adaptor
     * @return the schema map
     */
    static std::shared_ptr<SchemaMap> read_schemas(ArchiveReaderAdaptor& archives_dir);

    /**
     * Gets the variable dictionary reader for an archive
     * @param adaptor
     * @return the variable dictionary reader
     */
    static std::shared_ptr<VariableDictionaryReader> get_variable_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Gets the log type dictionary reader for an archive
     * @param adaptor
     * @return the log type dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_log_type_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Gets the array dictionary reader for an archive
     * @param adaptor
     * @return the array dictionary reader
     */
    static std::shared_ptr<LogTypeDictionaryReader> get_array_dictionary_reader(
            ArchiveReaderAdaptor& adaptor
    );

    /**
     * Converts a serialized 64-bit numeric value into `size_t` with bounds checking.
     *
     * Archive metadata fields are serialized using fixed-width integer types. If an archive writer
     * running on a 64-bit platform serialized values using `size_t`, a reader on a platform with a
     * narrower pointer type (e.g. wasm32) also using `size_t` could misinterpret the value due to
     * truncation.
     *
     * This helper assumes a 64-bit archive writer and verifies that a deserialized 64-bit value can
     * be safely represented as `size_t` before it is used for local buffer or container sizes.
     *
     * In the long term, the archive format should record explicit numeric serialization widths
     * rather than relying on platform-sized types.
     *
     * @param value The 64-bit value deserialized from archive metadata.
     * @return Converted `size_t` on success.
     * @return std::errc::value_too_large if the value cannot fit in `size_t`.
     */
    [[nodiscard]] static auto try_uint64_to_size_t(uint64_t value)
            -> ystdlib::error_handling::Result<size_t> {
        if (value > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            return std::errc::value_too_large;
        }
        return static_cast<size_t>(value);
    }

private:
    /**
     * Appends a column to the given schema reader
     * @param reader
     * @param column_id
     * @param schema_tree
     * @param var_dict
     * @param log_dict
     * @param array_dict
     * @param timestamp_dict
     * @return the appended reader column
     */
    static BaseColumnReader* append_reader_column(
            SchemaReader* reader,
            int32_t column_id,
            std::shared_ptr<SchemaTree> const& schema_tree,
            std::shared_ptr<VariableDictionaryReader> const& var_dict,
            std::shared_ptr<LogTypeDictionaryReader> const& log_dict,
            std::shared_ptr<LogTypeDictionaryReader> const& array_dict,
            std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict
    );
};
}  // namespace clp_s

#endif  // CLP_S_READERUTILS_HPP

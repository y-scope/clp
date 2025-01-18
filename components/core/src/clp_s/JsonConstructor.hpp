#ifndef CLP_S_JSONCONSTRUCTOR_HPP
#define CLP_S_JSONCONSTRUCTOR_HPP

#include <optional>
#include <set>
#include <string>
#include <utility>

#include "ArchiveReader.hpp"
#include "ColumnReader.hpp"
#include "DictionaryReader.hpp"
#include "ErrorCode.hpp"
#include "FileWriter.hpp"
#include "InputConfig.hpp"
#include "SchemaReader.hpp"
#include "SchemaTree.hpp"
#include "TraceableException.hpp"

namespace clp_s {
struct MetadataDbOption {
    MetadataDbOption(std::string const& uri, std::string const& collection)
            : mongodb_uri{uri},
              mongodb_collection{collection} {}

    std::string mongodb_uri;
    std::string mongodb_collection;
};

struct JsonConstructorOption {
    Path archive_path{};
    NetworkAuthOption network_auth{};
    std::string output_dir;
    bool ordered{false};
    bool print_ordered_chunk_stats{false};
    size_t target_ordered_chunk_size{};
    std::optional<MetadataDbOption> metadata_db{std::nullopt};
};

class JsonConstructor {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException(error_code, filename, line_number),
                  m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] char const* what() const noexcept override { return m_message.c_str(); }

    private:
        std::string m_message;
    };

    // Constructors
    explicit JsonConstructor(JsonConstructorOption const& option);

    /**
     * Decompresses each archive and stores the decompressed files in the output directory
     */
    void store();

private:
    /**
     * Reads all of the tables from m_archive_reader and writes all of the records
     * they contain to writer in log order.
     */
    void construct_in_order();

    JsonConstructorOption m_option{};
    std::unique_ptr<ArchiveReader> m_archive_reader;
};
}  // namespace clp_s

#endif  // CLP_S_JSONCONSTRUCTOR_HPP

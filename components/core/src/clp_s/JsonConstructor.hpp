#ifndef CLP_S_JSONCONSTRUCTOR_HPP
#define CLP_S_JSONCONSTRUCTOR_HPP

#include <set>
#include <string>
#include <utility>

#include "ArchiveReader.hpp"
#include "ColumnReader.hpp"
#include "DictionaryReader.hpp"
#include "ErrorCode.hpp"
#include "SchemaReader.hpp"
#include "SchemaTree.hpp"
#include "TraceableException.hpp"

namespace clp_s {
struct JsonConstructorOption {
    std::string archives_dir;
    std::string output_dir;
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
     * Reads the schema information
     */
    void construct();

    /**
     * Decompresses each archive and stores the decompressed files in the output directory
     */
    void store();

    /**
     * Closes the JsonConstructor
     */
    void close();

private:
    std::string m_archives_dir;
    std::string m_output_dir;

    std::unique_ptr<ArchiveReader> m_archive_reader;
    std::vector<std::string> m_archive_paths;
    size_t m_current_archive_index;
    size_t m_max_archive_index;

    std::shared_ptr<SchemaTree> m_schema_tree;
};
}  // namespace clp_s

#endif  // CLP_S_JSONCONSTRUCTOR_HPP

#ifndef GLT_GLOBALMYSQLMETADATADB_HPP
#define GLT_GLOBALMYSQLMETADATADB_HPP

#include "ErrorCode.hpp"
#include "GlobalMetadataDB.hpp"
#include "MySQLDB.hpp"
#include "TraceableException.hpp"

namespace glt {
/**
 * Class representing a MySQL global metadata database
 */
class GlobalMySQLMetadataDB : public GlobalMetadataDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "GlobalMySQLMetadataDB operation failed";
        }
    };

    class ArchiveIterator : public GlobalMetadataDB::ArchiveIterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            char const* what() const noexcept override {
                return "GlobalMySQLMetadataDB::ArchiveIterator operation failed";
            }
        };

        // Constructors
        explicit ArchiveIterator(MySQLDB::Iterator&& iterator)
                : m_db_iterator(std::make_unique<MySQLDB::Iterator>(std::move(iterator))) {}

        // Methods
        bool contains_element() const override { return m_db_iterator->contains_element(); }

        void get_next() override { m_db_iterator->get_next(); }

        void get_id(std::string& id) const override;

    private:
        // Variables
        std::unique_ptr<MySQLDB::Iterator> m_db_iterator;
    };

    // Constructors
    GlobalMySQLMetadataDB(
            std::string const& host,
            int port,
            std::string const& username,
            std::string const& password,
            std::string const& database_name,
            std::string const& table_prefix
    )
            : m_host(host),
              m_port(port),
              m_username(username),
              m_password(password),
              m_database_name(database_name),
              m_table_prefix(table_prefix) {}

    // Methods
    void open() override;
    void close() override;

    void
    add_archive(std::string const& id, streaming_archive::ArchiveMetadata const& metadata) override;
    void update_archive_metadata(
            std::string const& archive_id,
            streaming_archive::ArchiveMetadata const& metadata
    ) override;
    void update_metadata_for_files(
            std::string const& archive_id,
            std::vector<streaming_archive::writer::File*> const& files
    ) override;

    GlobalMetadataDB::ArchiveIterator* get_archive_iterator() override;
    GlobalMetadataDB::ArchiveIterator*
    get_archive_iterator_for_time_window(epochtime_t begin_ts, epochtime_t end_ts) override;
    GlobalMetadataDB::ArchiveIterator* get_archive_iterator_for_file_path(
            std::string const& file_path
    ) override;

private:
    // Variables
    std::string m_host;
    int m_port;
    std::string m_username;
    std::string m_password;
    std::string m_database_name;
    std::string m_table_prefix;

    MySQLDB m_db;

    std::unique_ptr<MySQLPreparedStatement> m_insert_archive_statement;
    std::unique_ptr<MySQLPreparedStatement> m_update_archive_size_statement;
    std::unique_ptr<MySQLPreparedStatement> m_upsert_file_statement;
};
}  // namespace glt

#endif  // GLT_GLOBALMYSQLMETADATADB_HPP

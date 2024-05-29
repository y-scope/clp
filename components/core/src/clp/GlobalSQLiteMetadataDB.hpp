#ifndef CLP_GLOBALSQLITEMETADATADB_HPP
#define CLP_GLOBALSQLITEMETADATADB_HPP

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ErrorCode.hpp"
#include "GlobalMetadataDB.hpp"
#include "SQLiteDB.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Class representing a MySQL global metadata database
 */
class GlobalSQLiteMetadataDB : public GlobalMetadataDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "GlobalSQLiteMetadataDB operation failed";
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
                return "GlobalSQLiteMetadataDB::ArchiveIterator operation failed";
            }
        };

        // Constructors
        explicit ArchiveIterator(SQLiteDB& db);
        ArchiveIterator(SQLiteDB& db, std::string const& file_path);
        ArchiveIterator(SQLiteDB& db, epochtime_t begin_ts, epochtime_t end_ts);

        // Methods
        bool contains_element() const override;
        void get_next() override;
        void get_id(std::string& id) const override;

    private:
        // Variables
        SQLitePreparedStatement m_statement;
    };

    // Constructors
    GlobalSQLiteMetadataDB(std::string const& path) : m_path(path) {}

    GlobalSQLiteMetadataDB(epochtime_t begin_ts, epochtime_t end_ts) {}

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

    GlobalMetadataDB::ArchiveIterator* get_archive_iterator() override {
        return new ArchiveIterator(m_db);
    }

    GlobalMetadataDB::ArchiveIterator*
    get_archive_iterator_for_time_window(epochtime_t begin_ts, epochtime_t end_ts) override {
        return new ArchiveIterator(m_db, begin_ts, end_ts);
    }

    GlobalMetadataDB::ArchiveIterator* get_archive_iterator_for_file_path(std::string const& path
    ) override {
        return new ArchiveIterator(m_db, path);
    }

    bool get_file_split(
            std::string const& orig_file_id,
            size_t message_ix,
            std::string& archive_id,
            std::string& file_split_id
    ) override;

private:
    // Variables
    std::string m_path;

    SQLiteDB m_db;

    std::unique_ptr<SQLitePreparedStatement> m_insert_archive_statement;
    std::unique_ptr<SQLitePreparedStatement> m_update_archive_size_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_file_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_files_transaction_begin_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_files_transaction_end_statement;
};
}  // namespace clp

#endif  // CLP_GLOBALSQLITEMETADATADB_HPP

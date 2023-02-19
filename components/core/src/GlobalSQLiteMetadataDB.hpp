#ifndef GLOBALSQLITEMETADATADB_HPP
#define GLOBALSQLITEMETADATADB_HPP

// C++ standard libraries
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// Project headers
#include "ErrorCode.hpp"
#include "GlobalMetadataDB.hpp"
#include "SQLiteDB.hpp"
#include "TraceableException.hpp"

/**
 * Class representing a MySQL global metadata database
 */
class GlobalSQLiteMetadataDB : public GlobalMetadataDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "GlobalSQLiteMetadataDB operation failed";
        }
    };

    class ArchiveIterator : public GlobalMetadataDB::ArchiveIterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "GlobalSQLiteMetadataDB::ArchiveIterator operation failed";
            }
        };

        // Constructors
        explicit ArchiveIterator (SQLiteDB& db);
        ArchiveIterator (SQLiteDB& db, const std::string& file_path);

        // Methods
        bool contains_element () const override;
        void get_next () override;
        void get_id (std::string& id) const override;

    private:
        // Variables
        SQLitePreparedStatement m_statement;
    };

    // Constructors
    GlobalSQLiteMetadataDB (const std::string& path) : m_path(path) {}

    // Methods
    void open () override;
    void close () override;

    void add_archive (const std::string& id, uint64_t uncompressed_size, uint64_t size,
                      const std::string& creator_id, uint64_t creation_num) override;
    void update_archive_size (const std::string& archive_id, uint64_t uncompressed_size,
                              uint64_t size) override;
    void update_metadata_for_files (const std::string& archive_id, const std::vector<streaming_archive::writer::File*>& files) override;

    GlobalMetadataDB::ArchiveIterator* get_archive_iterator () override { return new ArchiveIterator(m_db); }
    GlobalMetadataDB::ArchiveIterator* get_archive_iterator_for_file_path (const std::string& path) override { return new ArchiveIterator(m_db, path); }

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

#endif // GLOBALSQLITEMETADATADB_HPP

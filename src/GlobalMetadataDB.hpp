#ifndef GLOBALMETADATADB_HPP
#define GLOBALMETADATADB_HPP

// C++ standard libraries
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// Project headers
#include "ErrorCode.hpp"
#include "SQLiteDB.hpp"
#include "streaming_archive/writer/File.hpp"
#include "TraceableException.hpp"

class GlobalMetadataDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "GlobalMetadataDB operation failed";
        }
    };

    class Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "GlobalMetadataDB::Iterator operation failed";
            }
        };

        // Constructors
        explicit Iterator (SQLitePreparedStatement statement);

        // Methods
        bool has_next ();
        void next ();

    protected:
        // Variables
        SQLitePreparedStatement m_statement;
    };

    class ArchiveIterator : public Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "GlobalMetadataDB::ArchiveIterator operation failed";
            }
        };

        // Constructors
        explicit ArchiveIterator (SQLiteDB& db);
        ArchiveIterator (SQLiteDB& db, const std::string& file_path);

        // Methods
        void get_id (std::string& id) const;
    };

    // Constructors
    GlobalMetadataDB () : m_is_open(false) {};

    // Methods
    void open (const std::string& path);
    void close ();

    void add_archive (const std::string& id, const std::string& storage_id, size_t uncompressed_size, size_t size, const std::string& creator_id,
                      size_t creation_num);
    void update_archive_size (const std::string& archive_id, size_t uncompressed_size, size_t size);
    void update_files (const std::string& archive_id, const std::vector<streaming_archive::writer::File*>& files);

    ArchiveIterator get_archive_iterator () { return ArchiveIterator(m_db); }
    ArchiveIterator get_archive_iterator_for_file_path (const std::string& path) { return ArchiveIterator(m_db, path); }

private:
    // Variables
    bool m_is_open;

    SQLiteDB m_db;

    std::unique_ptr<SQLitePreparedStatement> m_insert_archive_statement;
    std::unique_ptr<SQLitePreparedStatement> m_update_archive_size_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_file_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_files_transaction_begin_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_files_transaction_end_statement;
};

#endif // GLOBALMETADATADB_HPP

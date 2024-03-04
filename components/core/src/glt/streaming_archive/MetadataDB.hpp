#ifndef GLT_STREAMING_ARCHIVE_METADATADB_HPP
#define GLT_STREAMING_ARCHIVE_METADATADB_HPP

#include <memory>
#include <string>
#include <vector>

#include "../SQLiteDB.hpp"
#include "writer/File.hpp"

namespace glt::streaming_archive {
class MetadataDB {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::MetadataDB operation failed";
        }
    };

    class Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            char const* what() const noexcept override {
                return "MetadataDB::Iterator operation failed";
            }
        };

        // Constructors
        explicit Iterator(SQLitePreparedStatement statement);

        // Methods
        bool has_next() { return m_statement.is_row_ready(); }

        void next() { m_statement.step(); }

        void reset();

    protected:
        // Variables
        SQLitePreparedStatement m_statement;
    };

    class FileIterator : public Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            char const* what() const noexcept override {
                return "MetadataDB::ArchiveIterator operation failed";
            }
        };

        // Constructors
        explicit FileIterator(
                SQLiteDB& db,
                epochtime_t begin_timestamp,
                epochtime_t end_timestamp,
                std::string const& file_path,
                bool in_specific_segment,
                segment_id_t segment_id
        );

        // Methods
        void set_segment_id(segment_id_t segment_id);

        void get_id(std::string& id) const;
        void get_orig_file_id(std::string& id) const;
        void get_path(std::string& path) const;
        epochtime_t get_begin_ts() const;
        epochtime_t get_end_ts() const;
        void get_timestamp_patterns(std::string& timestamp_patterns) const;
        size_t get_num_uncompressed_bytes() const;
        size_t get_num_messages() const;
        size_t get_num_variables() const;
        bool is_split() const;
        size_t get_split_ix() const;
        segment_id_t get_segment_id() const;

        // GLT specific
        size_t get_segment_logtypes_pos() const;
        size_t get_segment_offset_pos() const;
    };

    class EmptyDirectoryIterator : public Iterator {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                    : TraceableException(error_code, filename, line_number) {}

            // Methods
            char const* what() const noexcept override {
                return "MetadataDB::EmptyDirectoryIterator operation failed";
            }
        };

        // Constructors
        explicit EmptyDirectoryIterator(SQLiteDB& db);

        // Methods
        void get_path(std::string& path) const { m_statement.column_string(0, path); }
    };

    // Constructors
    MetadataDB() : m_is_open(false) {}

    // Methods
    void open(std::string const& path);
    void close();

    void update_files(std::vector<writer::File*> const& files);
    void add_empty_directories(std::vector<std::string> const& empty_directory_paths);

    std::unique_ptr<FileIterator> get_file_iterator(
            epochtime_t begin_ts,
            epochtime_t end_ts,
            std::string const& file_path,
            bool in_specific_segment,
            segment_id_t segment_id
    ) {
        return std::make_unique<FileIterator>(
                m_db,
                begin_ts,
                end_ts,
                file_path,
                in_specific_segment,
                segment_id
        );
    }

    std::unique_ptr<EmptyDirectoryIterator> get_empty_directory_iterator() {
        return std::make_unique<EmptyDirectoryIterator>(m_db);
    }

private:
    // Variables
    bool m_is_open;

    SQLiteDB m_db;
    std::unique_ptr<SQLitePreparedStatement> m_transaction_begin_statement;
    std::unique_ptr<SQLitePreparedStatement> m_transaction_end_statement;
    std::unique_ptr<SQLitePreparedStatement> m_upsert_file_statement;
    std::unique_ptr<SQLitePreparedStatement> m_insert_empty_directories_statement;
};
}  // namespace glt::streaming_archive

#endif  // GLT_STREAMING_ARCHIVE_METADATADB_HPP

#ifndef STREAMING_ARCHIVE_METADATADB_HPP
#define STREAMING_ARCHIVE_METADATADB_HPP

// C standard libraries

// C++ standard libraries
#include <memory>
#include <string>
#include <vector>

// Project headers
#include "../SQLiteDB.hpp"
#include "../type_utils.hpp"
#include "writer/File.hpp"

namespace streaming_archive {
    class MetadataDB {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::MetadataDB operation failed";
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
                    return "MetadataDB::Iterator operation failed";
                }
            };

            // Constructors
            explicit Iterator (SQLitePreparedStatement statement);

            // Methods
            bool has_next () { return m_statement.is_row_ready(); }
            void next () { m_statement.step(); }
            void reset ();

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
                OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

                // Methods
                const char* what () const noexcept override {
                    return "MetadataDB::ArchiveIterator operation failed";
                }
            };

            // Constructors
            explicit FileIterator (MetadataDB* m_db_ptr, SQLiteDB& db, epochtime_t begin_timestamp, epochtime_t end_timestamp, const std::string& file_path, bool in_specific_segment,
                                   segment_id_t segment_id);
            // Destructor
            // Need at least one virtual function to enable dynamic casting
            virtual ~FileIterator() {}
            // Methods
            void set_segment_id (segment_id_t segment_id);

            void get_id (std::string& id) const;
            void get_orig_file_id (std::string& id) const;
            void get_path (std::string& path) const;
            epochtime_t get_begin_ts () const;
            epochtime_t get_end_ts () const;
            void get_timestamp_patterns (std::string& timestamp_patterns) const;
            size_t get_num_uncompressed_bytes () const;
            size_t get_num_messages () const;
            size_t get_num_variables () const;
            bool is_split () const;
            size_t get_split_ix () const;
            segment_id_t get_segment_id () const;
        };

        class EmptyDirectoryIterator : public Iterator {
        public:
            // Types
            class OperationFailed : public TraceableException {
            public:
                // Constructors
                OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

                // Methods
                const char* what () const noexcept override {
                    return "MetadataDB::EmptyDirectoryIterator operation failed";
                }
            };

            // Constructors
            explicit EmptyDirectoryIterator (SQLiteDB& db);

            // Methods
            void get_path (std::string& path) const {
                m_statement.column_string(0, path);
            }
        };

        // Constructors
        MetadataDB () : m_is_open(false) {}

        // Methods
        void open (const std::string& path);
        void close ();

        void update_files (const std::vector<writer::File*>& files);
        void add_empty_directories (const std::vector<std::string>& empty_directory_paths);

        virtual std::unique_ptr<FileIterator> get_file_iterator (epochtime_t begin_ts, epochtime_t end_ts, const std::string& file_path, bool in_specific_segment,
                                                                 segment_id_t segment_id)
        {
            return std::make_unique<FileIterator>(this, m_db, begin_ts, end_ts, file_path, in_specific_segment, segment_id);
        }
        std::unique_ptr<EmptyDirectoryIterator> get_empty_directory_iterator () { return std::make_unique<EmptyDirectoryIterator>(m_db); }

        SQLitePreparedStatement get_files_select_statement (SQLiteDB& db, epochtime_t ts_begin, epochtime_t ts_end, const std::string& file_path, bool in_specific_segment, segment_id_t segment_id);

    protected:
        // Methods
        virtual size_t get_field_size() = 0;
        virtual void add_storage_specific_fields(std::vector<std::string>& field_names) = 0;
        virtual void bind_storage_specific_fields(writer::File*) = 0;
        virtual void add_storage_specific_field_names_and_types(std::vector<std::pair<std::string, std::string>>& file_field_names_and_types) = 0;
        virtual void add_storage_specific_ordering(std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) = 0;
        virtual void create_storage_specific_index(std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) = 0;

        void create_tables (const std::vector<std::pair<std::string, std::string>>& file_field_names_and_types, SQLiteDB& db);

        // Types
        enum class FilesTableFieldIndexes : uint16_t {
            Id = 0,  // NOTE: This needs to be the first item in the list
            OrigFileId,
            Path,
            BeginTimestamp,
            EndTimestamp,
            TimestampPatterns,
            NumUncompressedBytes,
            NumMessages,
            NumVariables,
            IsSplit,
            SplitIx,
            SegmentId,
            Length,
        };

        // Variables
        bool m_is_open;

        SQLiteDB m_db;
        std::unique_ptr<SQLitePreparedStatement> m_transaction_begin_statement;
        std::unique_ptr<SQLitePreparedStatement> m_transaction_end_statement;
        std::unique_ptr<SQLitePreparedStatement> m_upsert_file_statement;
        std::unique_ptr<SQLitePreparedStatement> m_insert_empty_directories_statement;
    };
}

#endif // STREAMING_ARCHIVE_METADATADB_HPP

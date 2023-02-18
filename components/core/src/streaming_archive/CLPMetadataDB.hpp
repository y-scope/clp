#ifndef CLPMETADATADB_HPP
#define CLPMETADATADB_HPP

#include "MetadataDB.hpp"
namespace streaming_archive::clp {
    class CLPMetadataDB : public MetadataDB {
    public:
        class CLPFileIterator : public FileIterator {
        public:
            // Types
            class OperationFailed : public TraceableException {
            public:
                // Constructors
                OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

                // Methods
                const char* what () const noexcept override {
                    return "GLTMetadataDB::GLTFileIterator operation failed";
                }
            };

            // Constructors
            explicit CLPFileIterator (MetadataDB* m_db_ptr, SQLiteDB& db, epochtime_t begin_timestamp, epochtime_t end_timestamp, const std::string& file_path, bool in_specific_segment,
                                      segment_id_t segment_id);

            // Methods
            size_t get_segment_timestamps_pos() const;
            size_t get_segment_logtypes_pos () const;
            size_t get_segment_variables_pos() const;

        };

        virtual std::unique_ptr<FileIterator> get_file_iterator (epochtime_t begin_ts, epochtime_t end_ts, const std::string& file_path, bool in_specific_segment,
                                                                 segment_id_t segment_id)
        {
            return std::make_unique<CLPFileIterator>(this, m_db, begin_ts, end_ts, file_path, in_specific_segment, segment_id);
        }

    private:
        // Types
        enum class CLPFilesTableFieldIndexes : uint16_t {
            SegmentTimestampsPosition = enum_to_underlying_type(FilesTableFieldIndexes::Length),
            SegmentLogtypesPosition,
            SegmentVariablesPosition,
            Length,
        };

        size_t get_field_size() override { return enum_to_underlying_type(CLPFilesTableFieldIndexes::Length); }
        void add_storage_specific_field_names_and_types(std::vector<std::pair<std::string, std::string>>& file_field_names_and_types) override;
        void add_storage_specific_fields(std::vector<std::string>& field_names) override;
        void bind_storage_specific_fields(writer::File *) override;
        void create_storage_specific_index(std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) override;
        void add_storage_specific_ordering(std::back_insert_iterator<fmt::memory_buffer> statement_buffer_ix) override;
    };
}

#endif //CLPMETADATADB_HPP

#ifndef CLP_GLOBALMETADATADB_HPP
#define CLP_GLOBALMETADATADB_HPP

#include <string>
#include <vector>

#include "streaming_archive/ArchiveMetadata.hpp"
#include "streaming_archive/MetadataDB.hpp"
#include "streaming_archive/writer/File.hpp"

namespace clp {
/**
 * Base class for a representation of the global metadata database
 */
class GlobalMetadataDB {
public:
    // Types
    class ArchiveIterator {
    public:
        // Destructor
        virtual ~ArchiveIterator() = default;

        // Methods
        virtual bool contains_element() const = 0;
        virtual void get_next() = 0;
        virtual void get_id(std::string& id) const = 0;
    };

    // Constructors
    GlobalMetadataDB() : m_is_open(false) {}

    // Destructor
    virtual ~GlobalMetadataDB() = default;

    // Methods
    /**
     * Opens the global metadata database
     */
    virtual void open() = 0;
    /**
     * Closes the global metadata database
     */
    virtual void close() = 0;

    /**
     * Adds an archive to the global metadata database
     * @param id
     * @param metadata
     */
    virtual void
    add_archive(std::string const& id, streaming_archive::ArchiveMetadata const& metadata)
            = 0;
    /**
     * Copies the metadata for all files in the given archive-level metadata database to the
     * global metadata database.
     * @param archive_id
     * @param archive_metadata_db
     */
    virtual void copy_metadata_for_files_from_archive_metadata_db(
            std::string const& archive_id,
            streaming_archive::MetadataDB& archive_metadata_db
    ) = 0;

    /**
     * Gets an iterator to iterate over every archive in the global metadata database
     * @return The archive iterator
     */
    virtual ArchiveIterator* get_archive_iterator() = 0;
    /**
     * Gets an iterator to iterate over every archive that falls in the given time window in the
     * global metadata database
     * @param begin_ts
     * @param end_ts
     * @return The archive iterator
     */
    virtual ArchiveIterator*
    get_archive_iterator_for_time_window(epochtime_t begin_ts, epochtime_t end_ts)
            = 0;
    /**
     * Gets an iterator to iterate over every archive that contains a given file path in the global
     * metadata database
     * @return The archive iterator
     */
    virtual ArchiveIterator* get_archive_iterator_for_file_path(std::string const& path) = 0;

    /**
     * Gets the file split that corresponds to the given message index in an original file.
     * @param file_orig_id
     * @param message_ix
     * @param archive_id Returns the ID of the archive containing the file split.
     * @param file_split_id Returns the ID of the file split.
     * @return Whether a matching file split was found.
     */
    virtual bool get_file_split(
            std::string const& orig_file_id,
            size_t message_ix,
            std::string& archive_id,
            std::string& file_split_id
    ) = 0;

protected:
    // Variables
    bool m_is_open;
};
}  // namespace clp

#endif  // CLP_GLOBALMETADATADB_HPP

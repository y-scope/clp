#ifndef GLOBALMETADATADB_HPP
#define GLOBALMETADATADB_HPP

// C++ standard libraries
#include <string>
#include <vector>

// Project headers
#include "streaming_archive/ArchiveMetadata.hpp"
#include "streaming_archive/writer/File.hpp"

/**
 * Base class for a representation of the global metadata database
 */
class GlobalMetadataDB {
public:
    // Types
    class ArchiveIterator {
    public:
        // Destructor
        virtual ~ArchiveIterator () = default;

        // Methods
        virtual bool contains_element () const = 0;
        virtual void get_next () = 0;
        virtual void get_id (std::string& id) const = 0;
    };

    // Constructors
    GlobalMetadataDB () : m_is_open(false) {}

    // Destructor
    virtual ~GlobalMetadataDB () = default;

    // Methods
    /**
     * Opens the global metadata database
     */
    virtual void open () = 0;
    /**
     * Closes the global metadata database
     */
    virtual void close () = 0;

    /**
     * Adds an archive to the global metadata database
     * @param id
     * @param metadata
     */
    virtual void add_archive (const std::string& id, const streaming_archive::ArchiveMetadata& metadata) = 0;
    /**
     * Updates the size of the archive identified by the given ID in the global metadata database
     * @param archive_id
     * @param metadata
     */
    virtual void update_archive_metadata (const std::string& archive_id, const streaming_archive::ArchiveMetadata& metadata) = 0;
    /**
     * Updates the metadata of the given files in the global metadata database
     * @param archive_id
     * @param files
     */
    virtual void update_metadata_for_files (const std::string& archive_id, const std::vector<streaming_archive::writer::File*>& files) = 0;

    /**
     * Gets an iterator to iterate over every archive in the global metadata database
     * @return The archive iterator
     */
    virtual ArchiveIterator* get_archive_iterator () = 0;
    /**
     * Gets an iterator to iterate over every archive that falls in the given time window in the global metadata database
     * @param begin_ts
     * @param end_ts
     * @return The archive iterator
     */
    virtual ArchiveIterator* get_archive_iterator_for_time_window (epochtime_t begin_ts, epochtime_t end_ts) = 0;
    /**
     * Gets an iterator to iterate over every archive that contains a given file path in the global metadata database
     * @return The archive iterator
     */
    virtual ArchiveIterator* get_archive_iterator_for_file_path (const std::string& path) = 0;

protected:
    // Variables
    bool m_is_open;
};

#endif // GLOBALMETADATADB_HPP

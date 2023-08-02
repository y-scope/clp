#ifndef TIMESTAMPDICTIONARYREADER_HPP
#define TIMESTAMPDICTIONARYREADER_HPP

#include "Defs.h"
#include "DictionaryReader.hpp"
#include "TimestampDictionaryEntry.hpp"

/**
 * Class for reading timestamp dictionaries from disk and performing operations on them
 */
class TimestampDictionaryReader : public DictionaryReader<timestamp_dictionary_id_t , TimestampDictionaryEntry> {
public:
    // Constructor
    TimestampDictionaryReader () : DictionaryReader<timestamp_dictionary_id_t, TimestampDictionaryEntry> (false) {}
};

#endif

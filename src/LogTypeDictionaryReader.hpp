#ifndef LOGTYPEDICTIONARYREADER_HPP
#define LOGTYPEDICTIONARYREADER_HPP

// Project headers
#include "Defs.h"
#include "DictionaryReader.hpp"
#include "LogTypeDictionaryEntry.hpp"

/**
 * Class for reading logtype dictionaries from disk and performing operations on them
 */
class LogTypeDictionaryReader : public DictionaryReader<logtype_dictionary_id_t, LogTypeDictionaryEntry> {};

#endif // LOGTYPEDICTIONARYREADER_HPP

#ifndef GLT_LOGTYPEDICTIONARYREADER_HPP
#define GLT_LOGTYPEDICTIONARYREADER_HPP

#include "Defs.h"
#include "DictionaryReader.hpp"
#include "LogTypeDictionaryEntry.hpp"

namespace glt {
/**
 * Class for reading logtype dictionaries from disk and performing operations on them
 */
class LogTypeDictionaryReader
        : public DictionaryReader<logtype_dictionary_id_t, LogTypeDictionaryEntry> {};
}  // namespace glt

#endif  // GLT_LOGTYPEDICTIONARYREADER_HPP

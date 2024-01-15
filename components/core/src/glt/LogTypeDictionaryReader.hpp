#ifndef CLP_LOGTYPEDICTIONARYREADER_HPP
#define CLP_LOGTYPEDICTIONARYREADER_HPP

#include "Defs.h"
#include "DictionaryReader.hpp"
#include "LogTypeDictionaryEntry.hpp"

namespace clp {
/**
 * Class for reading logtype dictionaries from disk and performing operations on them
 */
class LogTypeDictionaryReader
        : public DictionaryReader<logtype_dictionary_id_t, LogTypeDictionaryEntry> {};
}  // namespace clp

#endif  // CLP_LOGTYPEDICTIONARYREADER_HPP

#ifndef GLT_VARIABLEDICTIONARYREADER_HPP
#define GLT_VARIABLEDICTIONARYREADER_HPP

#include "Defs.h"
#include "DictionaryReader.hpp"
#include "VariableDictionaryEntry.hpp"

namespace glt {
/**
 * Class for reading variable dictionaries from disk and performing operations on them
 */
class VariableDictionaryReader
        : public DictionaryReader<variable_dictionary_id_t, VariableDictionaryEntry> {};
}  // namespace glt

#endif  // GLT_VARIABLEDICTIONARYREADER_HPP

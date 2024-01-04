#ifndef CLP_VARIABLEDICTIONARYREADER_HPP
#define CLP_VARIABLEDICTIONARYREADER_HPP

#include "Defs.h"
#include "DictionaryReader.hpp"
#include "VariableDictionaryEntry.hpp"

namespace clp {
/**
 * Class for reading variable dictionaries from disk and performing operations on them
 */
class VariableDictionaryReader
        : public DictionaryReader<variable_dictionary_id_t, VariableDictionaryEntry> {};
}  // namespace clp

#endif  // CLP_VARIABLEDICTIONARYREADER_HPP

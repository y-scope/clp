#ifndef VARIABLEDICTIONARYREADER_HPP
#define VARIABLEDICTIONARYREADER_HPP

// Project headers
#include "Defs.h"
#include "DictionaryReader.hpp"
#include "VariableDictionaryEntry.hpp"

/**
 * Class for reading variable dictionaries from disk and performing operations on them
 */
class VariableDictionaryReader : public DictionaryReader<variable_dictionary_id_t , VariableDictionaryEntry> {};

#endif // VARIABLEDICTIONARYREADER_HPP

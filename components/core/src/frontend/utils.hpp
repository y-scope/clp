#ifndef FRONTEND_UTILS_HPP
#define FRONTEND_UTILS_HPP

// Project headers
#include "Lexer.hpp"

namespace frontend {
    /**
     * Loads the lexer from the schema file at the given path
     * @param schema_file_path
     * @param reverse Whether to generate a reverse lexer
     * @param lexer
     */
    void load_lexer_from_file (const std::string& schema_file_path, bool reverse, Lexer& lexer);
}

#endif //FRONTEND_UTILS_HPP

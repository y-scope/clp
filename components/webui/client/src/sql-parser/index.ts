/* eslint-disable max-classes-per-file */
import {
    CharStream,
    CommonTokenStream,
    ErrorListener,
    Recognizer,
} from "antlr4";

import SqlBaseLexer from "./generated/SqlBaseLexer";
import SqlBaseParser from "./generated/SqlBaseParser";


class SyntaxError extends Error {
}

class SyntaxErrorListener<TSymbol> extends ErrorListener<TSymbol> {
    // eslint-disable-next-line max-params, class-methods-use-this
    override syntaxError (
        _recognizer: Recognizer<TSymbol>,
        _offendingSymbol: TSymbol,
        line: number,
        column: number,
        msg: string
    ) {
        throw new SyntaxError(`line ${line}:${column}: ${msg}`);
    }
}

/**
 * Validate a SQL string for syntax errors.
 *
 * @param sqlString
 * @throws {SyntaxError} with line, column, and message details if a syntax error is found.
 */
const validate = (sqlString: string) => {
    const syntaxErrorListener = new SyntaxErrorListener();
    const lexer = new SqlBaseLexer(new CharStream(sqlString.toUpperCase()));
    lexer.removeErrorListeners();
    lexer.addErrorListener(syntaxErrorListener);
    const parser = new SqlBaseParser(new CommonTokenStream(lexer));
    parser.removeErrorListeners();
    parser.addErrorListener(syntaxErrorListener);
    parser.singleStatement();
};

export {
    SyntaxError, validate,
};

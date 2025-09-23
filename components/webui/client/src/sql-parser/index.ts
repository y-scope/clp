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
        msg: string,
    ) {
        throw new SyntaxError(`line ${line}:${column}: ${msg}`);
    }
}

const LOWER_CASE_A_CHAR_CODE = 97;
const LOWER_CASE_Z_CHAR_CODE = 122;
const ASCII_CASE_OFFSET = 32;

class UpperCaseCharStream extends CharStream {
    // Override
    override LA (offset: number): number {
        // eslint-disable-next-line new-cap
        const c = super.LA(offset);
        if (0 >= c) {
            return c;
        }
        if (LOWER_CASE_A_CHAR_CODE <= c && LOWER_CASE_Z_CHAR_CODE >= c) {
            return c - ASCII_CASE_OFFSET;
        }

        return c;
    }
}

/**
 * Creates a SQL parser for a given input string.
 *
 * @param input The SQL query string to be parsed.
 * @return The configured SQL parser instance ready to parse the input.
 */
const buildParser = (input: string): SqlBaseParser => {
    const syntaxErrorListener = new SyntaxErrorListener();
    const lexer = new SqlBaseLexer(new UpperCaseCharStream(input));
    lexer.removeErrorListeners();
    lexer.addErrorListener(syntaxErrorListener);
    const parser = new SqlBaseParser(new CommonTokenStream(lexer));
    parser.removeErrorListeners();
    parser.addErrorListener(syntaxErrorListener);

    return parser;
};

/**
 * Validate a SQL string for syntax errors.
 *
 * @param sqlString
 * @throws {SyntaxError} with line, column, and message details if a syntax error is found.
 */
const validate = (sqlString: string) => {
    buildParser(sqlString).singleStatement();
};

interface BuildSearchQueryProps {
    selectItemList: string;
    relationList: string;
    booleanExpression?: string | undefined;
    sortItemList?: string | undefined;
    limitValue?: string | undefined;
}

/**
 * Constructs a SQL search query string from a set of structured components.
 *
 * @param props
 * @param props.selectItemList
 * @param props.relationList
 * @param props.booleanExpression
 * @param props.sortItemList
 * @param props.limitValue
 * @return
 * @throws {Error} if the constructed SQL string is not valid.
 */
const buildSearchQuery = ({
    selectItemList,
    relationList,
    booleanExpression,
    sortItemList,
    limitValue,
}: BuildSearchQueryProps): string => {
    let sqlString = `SELECT ${selectItemList} FROM ${relationList}`;
    if ("undefined" !== typeof booleanExpression) {
        sqlString += ` WHERE ${booleanExpression}`;
    }
    if ("undefined" !== typeof sortItemList) {
        sqlString += ` ORDER BY ${sortItemList}`;
    }
    if ("undefined" !== typeof limitValue) {
        sqlString += ` LIMIT ${limitValue}`;
    }

    try {
        validate(sqlString);
    } catch (err: unknown) {
        throw new Error(`The constructed SQL is not valid: ${sqlString}`, {cause: err});
    }

    return sqlString;
};

export {
    buildSearchQuery,
    SyntaxError,
    validate,
};

export type {BuildSearchQueryProps};

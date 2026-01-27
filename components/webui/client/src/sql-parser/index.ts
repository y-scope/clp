/* eslint-disable max-classes-per-file */
import {
    CharStream,
    CommonTokenStream,
    ErrorListener,
    Recognizer,
    Token,
} from "antlr4";

import SqlLexer from "./generated/SqlLexer";
import SqlParser from "./generated/SqlParser";
import {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
} from "./typings";


class SyntaxError extends Error {
}

interface ValidationError {
    line: number;
    column: number;
    message: string;
    startColumn: number;
    endColumn: number;
}

class SyntaxErrorListener<TSymbol> extends ErrorListener<TSymbol> {
    errors: ValidationError[] = [];

    // eslint-disable-next-line max-params
    override syntaxError (
        _recognizer: Recognizer<TSymbol>,
        offendingSymbol: TSymbol,
        line: number,
        column: number,
        msg: string,
    ) {
        // Offending symbol always returns a token
        // https://github.com/antlr/antlr4/blob/cc82115a4e7f53d71d9d905caa2c2dfa4da58899/runtime/JavaScript/src/antlr4/Parser.js#L308
        const token = offendingSymbol as unknown as Token;
        const startColumn = token.start + 1;
        const endColumn = token.stop + 2;

        this.errors.push({
            column: column,
            endColumn: endColumn,
            line: line,
            message: msg,
            startColumn: startColumn,
        });
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
 * Helper function to set up parser with error listener.
 *
 * @param sqlString
 * @return Object containing parser and error listener
 */
const setupParser = (sqlString: string) => {
    const syntaxErrorListener = new SyntaxErrorListener();
    const lexer = new SqlLexer(new UpperCaseCharStream(sqlString));
    lexer.removeErrorListeners();
    lexer.addErrorListener(syntaxErrorListener);
    const parser = new SqlParser(new CommonTokenStream(lexer));
    parser.removeErrorListeners();
    parser.addErrorListener(syntaxErrorListener);

    return {parser, syntaxErrorListener};
};

/**
 * Validate a SELECT item list and return any syntax errors found.
 *
 * @param sqlString
 * @return Array of validation errors, empty if valid
 */
const validateSelectItemList = (sqlString: string): ValidationError[] => {
    const {parser, syntaxErrorListener} = setupParser(sqlString);
    parser.standaloneSelectItemList();

    return syntaxErrorListener.errors;
};

/**
 * Validate a boolean expression and return any syntax errors found.
 *
 * @param sqlString
 * @return Array of validation errors, empty if valid
 */
const validateBooleanExpression = (sqlString: string): ValidationError[] => {
    const {parser, syntaxErrorListener} = setupParser(sqlString);
    parser.standaloneBooleanExpression();

    return syntaxErrorListener.errors;
};

/**
 * Validate a sort item list and return any syntax errors found.
 *
 * @param sqlString
 * @return Array of validation errors, empty if valid
 */
const validateSortItemList = (sqlString: string): ValidationError[] => {
    const {parser, syntaxErrorListener} = setupParser(sqlString);
    parser.standaloneSortItemList();

    return syntaxErrorListener.errors;
};

const MILLISECONDS_PER_SECOND = 1000;

/**
 * Constructs a SQL search query string from a set of structured components.
 *
 * @param props
 * @param props.selectItemList
 * @param props.databaseName
 * @param props.booleanExpression
 * @param props.sortItemList
 * @param props.limitValue
 * @param props.startTimestamp
 * @param props.endTimestamp
 * @param props.timestampKey
 * @return
 */
const buildSearchQuery = ({
    selectItemList,
    databaseName,
    booleanExpression,
    sortItemList,
    limitValue,
    startTimestamp,
    endTimestamp,
    timestampKey,
}: BuildSearchQueryProps): string => {
    let queryString = `SELECT ${selectItemList} FROM ${databaseName}
WHERE to_unixtime(${timestampKey}) BETWEEN
${startTimestamp.valueOf() / MILLISECONDS_PER_SECOND}
AND ${endTimestamp.valueOf() / MILLISECONDS_PER_SECOND}`;

    if ("undefined" !== typeof booleanExpression) {
        queryString += ` AND (${booleanExpression})`;
    }
    if ("undefined" !== typeof sortItemList) {
        queryString += `\nORDER BY ${sortItemList}`;
    }
    if ("undefined" !== typeof limitValue) {
        queryString += `\nLIMIT ${limitValue}`;
    }

    return queryString;
};


/**
 * Constructs a bucketed timeline query.
 *
 * @param props
 * @param props.databaseName
 * @param props.startTimestamp
 * @param props.endTimestamp
 * @param props.bucketCount
 * @param props.timestampKey
 * @param props.booleanExpression
 * @return
 */
const buildTimelineQuery = ({
    databaseName,
    booleanExpression,
    startTimestamp,
    endTimestamp,
    bucketCount,
    timestampKey,
}: BuildTimelineQueryProps) => {
    const step = (endTimestamp.valueOf() - startTimestamp.valueOf()) / bucketCount;
    const timestamps = Array.from(
        {length: bucketCount},
        (_, i) => Math.floor(startTimestamp.valueOf() + (i * step))
    );

    const booleanExpressionQuery = "undefined" === typeof booleanExpression ?
        "" :
        `AND (${booleanExpression})`;

    const queryString = `WITH buckets AS (
    SELECT
        width_bucket(
            to_unixtime(${timestampKey}),
            ${startTimestamp.valueOf() / MILLISECONDS_PER_SECOND},
            ${endTimestamp.valueOf() / MILLISECONDS_PER_SECOND},
            ${bucketCount}) AS idx,
        COUNT(*) AS cnt
    FROM ${databaseName}
    WHERE to_unixtime(${timestampKey}) BETWEEN
        ${startTimestamp.valueOf() / MILLISECONDS_PER_SECOND}
        AND ${endTimestamp.valueOf() / MILLISECONDS_PER_SECOND}
        ${booleanExpressionQuery}
    GROUP BY 1
    ORDER BY 1
),
timestamps AS (
    SELECT *
    FROM UNNEST(array [${timestamps.join(", ")}]) WITH ORDINALITY AS t(timestamp, idx)
)
SELECT
    COALESCE(cnt, 0) AS count,
    CAST(timestamp AS double) AS timestamp
FROM buckets
LEFT JOIN timestamps ON buckets.idx = timestamps.idx
ORDER BY timestamps.idx
`;

    return queryString;
};

export {
    buildSearchQuery,
    buildTimelineQuery,
    SyntaxError,
    validateBooleanExpression,
    validateSelectItemList,
    validateSortItemList,
};

export type {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
    ValidationError,
};

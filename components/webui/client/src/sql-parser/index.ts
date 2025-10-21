/* eslint-disable max-classes-per-file */
import {
    CharStream,
    CommonTokenStream,
    ErrorListener,
    Recognizer,
} from "antlr4";

import SqlLexer from "./generated/SqlLexer";
import SqlParser from "./generated/SqlParser";
import {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
} from "./typings";


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
const buildParser = (input: string): SqlParser => {
    const syntaxErrorListener = new SyntaxErrorListener();
    const lexer = new SqlLexer(new UpperCaseCharStream(input));
    lexer.removeErrorListeners();
    lexer.addErrorListener(syntaxErrorListener);
    const parser = new SqlParser(new CommonTokenStream(lexer));
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
 * @throws {Error} if the constructed SQL string is not valid.
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
WHERE to_unixtime(${timestampKey}) BETWEEN ${startTimestamp.unix()} AND ${endTimestamp.unix()}`;

    if ("undefined" !== typeof booleanExpression) {
        queryString += ` AND (${booleanExpression})`;
    }
    if ("undefined" !== typeof sortItemList) {
        queryString += `\nORDER BY ${sortItemList}`;
    }
    if ("undefined" !== typeof limitValue) {
        queryString += `\nLIMIT ${limitValue}`;
    }

    try {
        validate(queryString);
    } catch (err: unknown) {
        console.error(`The constructed SQL is not valid: ${queryString}`, err);
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
 * @throws {Error} if the constructed SQL string is not valid.
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
            ${startTimestamp.unix()},
            ${endTimestamp.unix()},
            ${bucketCount}) AS idx,
        COUNT(*) AS cnt
    FROM ${databaseName}
    WHERE to_unixtime(${timestampKey}) BETWEEN ${startTimestamp.unix()} AND ${endTimestamp.unix()}
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

    try {
        validate(queryString);
    } catch (err: unknown) {
        console.error(`The constructed SQL is not valid: ${queryString}`, err);
    }

    return queryString;
};

export {
    buildSearchQuery,
    buildTimelineQuery,
    SyntaxError,
    validate,
};

export type {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
};

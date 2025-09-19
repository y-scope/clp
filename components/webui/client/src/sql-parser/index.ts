/* eslint-disable max-classes-per-file */
import {
    CharStream,
    CommonTokenStream,
    ErrorListener,
    Recognizer,
} from "antlr4";

import SqlBaseLexer from "./generated/SqlBaseLexer";
import SqlBaseParser, {ColumnReferenceContext} from "./generated/SqlBaseParser";
import SqlBaseVisitor from "./generated/SqlBaseVisitor";


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

class TimestampKeyChecker extends SqlBaseVisitor<void> {
    hasTimestamp: boolean;

    constructor (timestampKey: string) {
        super();
        this.hasTimestamp = false;

        // Three possible terminal nodes of the `identifer` context
        const timestampIdentifiers = new Set([
            // unquotedIdentifier
            timestampKey,

            // quotedIdentifier
            `"${timestampKey}"`,

            // backQuotedIdentifier
            `\`${timestampKey}\``,
        ]);

        this.visitColumnReference = (ctx: ColumnReferenceContext) => {
            if (timestampIdentifiers.has(ctx.identifier().getText())) {
                this.hasTimestamp = true;
            }
        };
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

class InvalidBooleanExpressionError extends Error {
}

interface BuildSearchQueryProps {
    selectItemList: string;
    relationList: string;
    booleanExpression?: string | undefined;
    sortItemList?: string | undefined;
    limitValue?: string | undefined;
    startTimestamp: number;
    endTimestamp: number;
    timestampKey: string;
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
 * @param props.startTimestamp
 * @param props.endTimestamp
 * @param props.timestampKey
 * @return
 * @throws {Error} if the constructed SQL string is not valid.
 * @throws {InvalidBooleanExpressionError} if `booleanExpression` references the timestamp column.
 */
const buildSearchQuery = ({
    selectItemList,
    relationList,
    booleanExpression,
    sortItemList,
    limitValue,
    startTimestamp,
    endTimestamp,
    timestampKey,
}: BuildSearchQueryProps): string => {
    let queryString = `SELECT ${selectItemList} FROM ${relationList}
WHERE to_unixtime(${timestampKey}) BETWEEN ${startTimestamp} AND ${endTimestamp}`;

    if ("undefined" !== typeof booleanExpression) {
        const booleanExpressionTree = buildParser(booleanExpression).standaloneBooleanExpression()
            .booleanExpression();

        const checker = new TimestampKeyChecker(timestampKey);
        checker.visit(booleanExpressionTree);
        if (checker.hasTimestamp) {
            throw new InvalidBooleanExpressionError();
        }

        queryString += ` AND (${booleanExpression})`;
    }
    if ("undefined" !== typeof sortItemList) {
        queryString += ` ORDER BY ${sortItemList}`;
    }
    if ("undefined" !== typeof limitValue) {
        queryString += ` LIMIT ${limitValue}`;
    }

    try {
        validate(queryString);
    } catch (err: unknown) {
        throw new Error(`The constructed SQL is not valid: ${queryString}`, {cause: err});
    }

    return queryString;
};

interface BuildTimelineQueryProps {
    databaseName: string;
    startTimestamp: number;
    endTimestamp: number;
    bucketCount: number;
    timestampKey: string;
}

/**
 * Constructs a bucketed timeline query.
 *
 * @param props
 * @param props.databaseName
 * @param props.startTimestamp
 * @param props.endTimestamp
 * @param props.bucketCount
 * @param props.timestampKey
 * @return
 * @throws {Error} if the constructed SQL string is not valid.
 */
const buildTimelineQuery = ({
    databaseName,
    startTimestamp,
    endTimestamp,
    bucketCount,
    timestampKey,
}: BuildTimelineQueryProps) => {
    // Converting float to decimal can have precision errors.
    const startTimestampFixed = startTimestamp.toFixed(8);
    const endTimestampFixed = endTimestamp.toFixed(8);
    const queryString = `WITH filtered AS (
    SELECT to_unixtime(${timestampKey}) AS timestamp
    FROM ${databaseName}
    WHERE to_unixtime(${timestampKey}) BETWEEN ${startTimestampFixed} AND ${endTimestampFixed}
),
buckets AS (
    SELECT width_bucket(
        timestamp,
        ${startTimestampFixed},
        ${endTimestampFixed},
        ${bucketCount}
        ) AS index
    FROM filtered
),
bucket_count AS (
    SELECT index, COUNT(index) AS count
    FROM buckets
    WHERE index BETWEEN 1 AND ${bucketCount}
    GROUP BY index
),
bucket_timestamp AS (
    SELECT
        index + 1 AS index,
        ${startTimestampFixed}
        + (index*(${endTimestampFixed}-${startTimestampFixed})/${bucketCount}) AS timestamp
    FROM
        UNNEST(sequence(0, ${bucketCount - 1}, 1)) AS t(index)
)
SELECT
    COALESCE(bucket_count.count, 0) AS count,
    CAST(timestamp AS double) AS timestamp
FROM bucket_timestamp
LEFT JOIN bucket_count ON bucket_count.index = bucket_timestamp.index
ORDER BY bucket_count.index
`;

    try {
        validate(queryString);
    } catch (err: unknown) {
        throw new Error(`The constructed SQL is not valid: ${queryString}`, {cause: err});
    }

    return queryString;
};

export {
    buildSearchQuery,
    buildTimelineQuery,
    InvalidBooleanExpressionError,
    SyntaxError,
    validate,
};

export type {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
};

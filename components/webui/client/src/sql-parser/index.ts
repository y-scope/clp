/* eslint-disable max-classes-per-file */
import {Nullable} from "@webui/common/utility-types";
import {
    CharStream,
    CommonTokenStream,
    ErrorListener,
    ParseTree,
    Recognizer,
    TerminalNode,
} from "antlr4";

import SqlBaseLexer from "./generated/SqlBaseLexer";
import SqlBaseParser, {
    BooleanExpressionContext,
    QueryNoWithContext,
    QuerySpecificationContext,
    RelationListContext,
    SelectItemListContext,
    SortItemListContext,
} from "./generated/SqlBaseParser";
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

interface ModifierProps {
    selectItemList: SelectItemListContext;
    relationList: RelationListContext;
    booleanExpression: Nullable<BooleanExpressionContext>;
    sortItemList: Nullable<SortItemListContext>;
    limitValue: Nullable<TerminalNode>;
}

class TemplateTransformer extends SqlBaseVisitor<void> {
    constructor ({
        selectItemList,
        relationList,
        booleanExpression,
        sortItemList,
        limitValue,
    }: ModifierProps) {
        super();
        this.visitQuerySpecification = (ctx: QuerySpecificationContext) => {
            const children: ParseTree[] = [
                // eslint-disable-next-line new-cap
                ctx.SELECT(),
                selectItemList,
                // eslint-disable-next-line new-cap
                ctx.FROM(),
                relationList,
            ];

            if (null !== booleanExpression) {
                // eslint-disable-next-line new-cap
                children.push(ctx.WHERE(), booleanExpression);
            }
            ctx.children = children;
        };

        this.visitQueryNoWith = (ctx: QueryNoWithContext) => {
            this.visit(ctx.queryTerm());

            const children: ParseTree[] = [
                ctx.queryTerm(),
            ];

            if (null !== sortItemList) {
                // eslint-disable-next-line new-cap
                children.push(ctx.ORDER(), ctx.BY(), sortItemList);
            }
            if (null !== limitValue) {
                // eslint-disable-next-line new-cap
                children.push(ctx.LIMIT(), limitValue);
            }
            ctx.children = children;
        };
    }
}

class QueryFormatter extends SqlBaseVisitor<void> {
    tokens: Array<string> = [];

    override visitTerminal (node: TerminalNode) {
        this.tokens.push(node.getText());
    }
}

interface BuildSearchQueryProps {
    selectItemList: string;
    relationList: string;
    booleanExpression: Nullable<string>;
    sortItemList: Nullable<string>;
    limitValue: Nullable<string>;
}

const SEARCH_QUERY_TEMPLATE = "SELECT item FROM relation WHERE TRUE ORDER BY item LIMIT 1";

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
 * @throws {SyntaxError} if any of the input is not valid.
 * @throws {Error} if the constructed SQL string is not valid.
 */
const buildSearchQuery = ({
    selectItemList,
    relationList,
    booleanExpression,
    sortItemList,
    limitValue,
}: BuildSearchQueryProps): string => {
    const templateTree = buildParser(SEARCH_QUERY_TEMPLATE).queryNoWith();

    new TemplateTransformer({
        /* eslint-disable sort-keys */
        selectItemList: buildParser(selectItemList)
            .standaloneSelectItemList()
            .selectItemList(),
        relationList: buildParser(relationList)
            .standaloneRelationList()
            .relationList(),
        booleanExpression: null === booleanExpression ?
            null :
            buildParser(booleanExpression)
                .standaloneBooleanExpression()
                .booleanExpression(),
        sortItemList: null === sortItemList ?
            null :
            buildParser(sortItemList)
                .standaloneSortItemList()
                .sortItemList(),
        limitValue: null === limitValue ?
            null :
            buildParser(limitValue).standaloneIntegerValue()
            // eslint-disable-next-line new-cap
                .INTEGER_VALUE(),
        /* eslint-enable sort-keys */
    }).visit(templateTree);

    const formatter = new QueryFormatter();
    formatter.visit(templateTree);
    const sqlString = formatter.tokens.join(" ");
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

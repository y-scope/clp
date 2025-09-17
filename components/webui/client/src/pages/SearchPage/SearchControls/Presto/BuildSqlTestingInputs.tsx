import {FormEvent} from "react";

import {
    Static,
    Type,
} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";

import {buildSearchQuery} from "../../../../sql-parser";
import {handlePrestoQuerySubmit} from "./presto-search-requests";


// eslint-disable-next-line no-warning-comments
// TODO: Replace this with actual SQL inputs

const TransformEmptyStringSchema = Type.Transform(
    Type.Optional(Type.Union([
        Type.String(),
        Type.Undefined(),
    ]))
)
    .Decode((value) => (
        ("undefined" === typeof value || "" === value.trim()) ?
        // eslint-disable-next-line no-undefined
            undefined :
            value))
    .Encode((value) => value);

const BuildSearchQueryPropsSchema = Type.Object({
    /* eslint-disable sort-keys */
    selectItemList: Type.String(),
    relationList: Type.String(),
    booleanExpression: TransformEmptyStringSchema,
    sortItemList: TransformEmptyStringSchema,
    limitValue: TransformEmptyStringSchema,
    /* eslint-enable sort-keys */
});

/**
 * Returns a input boxes to test `buildSearchQuery`.
 *
 * @return
 */
const BuildSqlTestingInputs = () => {
    return (
        <form
            onSubmit={(ev: FormEvent<HTMLFormElement>) => {
                ev.preventDefault();
                const formData = new FormData(ev.target as HTMLFormElement);
                const props: Static<typeof BuildSearchQueryPropsSchema> = Value.Parse(
                    BuildSearchQueryPropsSchema,
                    Object.fromEntries(formData),
                );

                const sqlString = buildSearchQuery(props);
                console.log(`SQL: ${sqlString}`);
                handlePrestoQuerySubmit({queryString: sqlString});
            }}
        >
            <label>select:</label>
            <input name={"selectItemList"}/>
            <label>from:</label>
            <input name={"relationList"}/>
            <label>where:</label>
            <input name={"booleanExpression"}/>
            <label>order:</label>
            <input name={"sortItemList"}/>
            <label>limit:</label>
            <input name={"limitValue"}/>
            <button type={"submit"}>Run</button>
        </form>
    );
};

export {BuildSqlTestingInputs};

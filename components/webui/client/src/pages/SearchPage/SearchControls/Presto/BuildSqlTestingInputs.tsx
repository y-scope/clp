import {FormEvent} from "react";

import {
    Static,
    Type,
} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";

import {submitQuery} from "../../../../api/presto-search";
import {
    buildSearchQuery,
    buildTimelineQuery,
} from "../../../../sql-parser";
import {handlePrestoQuerySubmit} from "./presto-search-requests";


// eslint-disable-next-line no-warning-comments
// TODO: Replace this file with actual SQL inputs

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
    startTimestamp: Type.Number(),
    endTimestamp: Type.Number(),
    timestampKey: Type.String(),
    /* eslint-enable sort-keys */
});

const BuildTimelineQueryPropsSchema = Type.Object({
    databaseName: Type.String(),
    startTimestamp: Type.Number(),
    endTimestamp: Type.Number(),
    timestampKey: Type.String(),
});

/**
 * Returns input boxes to test `buildSearchQuery`.
 *
 * @return
 */
const BuildSqlTestingInputs = () => {
    return (
        <form
            onSubmit={(ev: FormEvent<HTMLFormElement>) => {
                ev.preventDefault();
                const formData = new FormData(ev.target as HTMLFormElement);
                const formDataObject = Object.fromEntries(formData);
                const props: Static<typeof BuildSearchQueryPropsSchema> = Value.Parse(
                    BuildSearchQueryPropsSchema,
                    formDataObject,
                );
                const queryString = buildSearchQuery(props);
                console.log(`SQL: ${queryString}`);
                handlePrestoQuerySubmit({queryString: queryString});

                const timelineProps: Static<typeof BuildTimelineQueryPropsSchema> = Value.Parse(
                    BuildTimelineQueryPropsSchema,
                    formDataObject
                );
                const timelineQueryString = buildTimelineQuery({
                    bucketCount: 10,
                    ...timelineProps,
                });

                console.log(`Timeline SQL: ${timelineQueryString}`);
                submitQuery({queryString: timelineQueryString})
                    .then((result) => {
                        const {searchJobId: aggregationJobId} = result.data;
                        console.log(`aggregationJobId: ${aggregationJobId}`);
                    })
                    .catch((err: unknown) => {
                        console.error(err);
                    });
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
            <br/>
            <label>database_name:</label>
            <input name={"databaseName"}/>
            <label>start_timestamp:</label>
            <input name={"startTimestamp"}/>
            <label>end_timestamp:</label>
            <input name={"endTimestamp"}/>
            <label>timestamp_key:</label>
            <input name={"timestampKey"}/>
            <button type={"submit"}>Run</button>
        </form>
    );
};

export {BuildSqlTestingInputs};

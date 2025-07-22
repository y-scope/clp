import {
    Static,
    Type,
} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";
import axios from "axios";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with shared type from the `@common` directory once refactoring is completed.
// Currently, server schema types require typebox dependency so they cannot be moved to the
// `@common` directory with current implementation.
const StringSchema = Type.String({
    minLength: 1,
});

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const PrestoSearchJobCreationSchema = Type.Object({
    queryString: StringSchema,
});

type PrestoSearchJobCreation = Static<typeof PrestoSearchJobCreationSchema>;

const PrestoJobSchema = Type.Object(
    {
        searchJobId: StringSchema,
    }
);

type PrestoJob = Static<typeof PrestoJobSchema>;

/**
 * Sends post request to server tosubmit presto query.
 *
 * @param payload
 * @return
 */
const submitQuery = async (payload: PrestoSearchJobCreation): Promise<PrestoJob> => {
    const ret = await axios.post<PrestoJob>("/api/presto-search/query", payload);
    return Value.Parse(PrestoJobSchema, ret.data);
};

export {submitQuery};

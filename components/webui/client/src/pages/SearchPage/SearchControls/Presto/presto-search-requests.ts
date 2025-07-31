import {
    type PrestoQueryJobCreationSchema,
    submitQuery,
} from "../../../../api/presto-search";


/**
 * Submits a new Presto query to server.
 *
 * @param payload
 */
const handlePrestoQuerySubmit = (payload: PrestoQueryJobCreationSchema) => {
    submitQuery(payload)
        .then((result) => {
            const {searchJobId} = result.data;
            console.debug(
                "Presto search job created - ",
                "Search job ID:",
                searchJobId
            );
        })
        .catch((err: unknown) => {
            console.error("Failed to submit query:", err);
        });
};

export {handlePrestoQuerySubmit};

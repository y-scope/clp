import {
    type PrestoQueryJobCreationSchema,
    submitQuery,
} from "../../../../api/presto-search";
import useSearchStore  from "../../SearchState/";


/**
 * Submits a new Presto query to server.
 *
 * @param payload
 */
const handlePrestoQuerySubmit = (payload: PrestoQueryJobCreationSchema) => {
    const store = useSearchStore.getState();
    submitQuery(payload)
        .then((result) => {
            const {searchJobId} = result.data;
            store.updateSearchJobId(searchJobId);
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

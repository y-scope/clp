import {
    useEffect,
    useRef,
    useState,
} from "react";

import {
    QUERY_LOAD_STATE,
    submitExtractIrJob,
} from "../api/query.js";
import Loading from "./Loading.jsx";


/**
 * Submits queries and renders the query states.
 *
 * @return {React.ReactElement}
 */
const Query = () => {
    const [queryState, setQueryState] = useState(QUERY_LOAD_STATE.SUBMITTING);
    const [errorMsg, setErrorMsg] = useState(null);
    const isFirstRun = useRef(true);

    useEffect(() => {
        if (false === isFirstRun.current) {
            return;
        }
        isFirstRun.current = false;

        const searchParams = new URLSearchParams(window.location.search);
        const origFileId = searchParams.get("origFileId");
        const logEventIx = searchParams.get("logEventIx");
        if (null === origFileId || null === logEventIx) {
            const error = "Either `origFileId` or `logEventIx` are missing from the URL " +
            "parameters. Note that non-IR-extraction queries are not supported at the moment.";

            console.error(error);
            setErrorMsg(error);
        }

        submitExtractIrJob(origFileId, Number(logEventIx), setQueryState, setErrorMsg).then();
    }, []);

    return (
        <Loading
            currentState={queryState}
            errorMsg={errorMsg}/>
    );
};

export default Query;

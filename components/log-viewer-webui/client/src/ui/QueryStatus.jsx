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
const QueryStatus = () => {
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
        const logEventIdx = searchParams.get("logEventIdx");
        if (null === origFileId || null === logEventIdx) {
            const error = "Either `origFileId` or `logEventIdx` are missing from the URL " +
            "parameters. Note that non-IR-extraction queries are not supported at the moment.";

            console.error(error);
            setErrorMsg(error);
        }

        submitExtractIrJob(origFileId, Number(logEventIdx), setQueryState, setErrorMsg).then();
    }, []);

    return (
        <Loading
            currentState={queryState}
            errorMsg={errorMsg}/>
    );
};

export default QueryStatus;

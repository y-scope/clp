import {
    useEffect,
    useRef,
    useState,
} from "react";

import {AxiosError} from "axios";

import {submitExtractStreamJob} from "../api/query.js";
import {QUERY_LOADING_STATES} from "../typings/query.js";
import Loading from "./Loading.jsx";


/* eslint-disable sort-keys */
let enumQueryType;
/**
 * Enum of job types, matching the `QueryJobType` class in
 * `job_orchestration.query_scheduler.constants`.
 *
 * @enum {number}
 */
const QUERY_JOB_TYPE = Object.freeze({
    SEARCH_OR_AGGREGATION: (enumQueryType = 0),
    EXTRACT_IR: ++enumQueryType,
    EXTRACT_JSON: ++enumQueryType,
});
/* eslint-enable sort-keys */

/**
 * Returns the extract job type based on the given stream type
 *
 * @param {string} streamType
 * @return {number|null}
 */
const getExtractJobType = (streamType) => {
    if ("ir" === streamType) {
        return QUERY_JOB_TYPE.EXTRACT_IR;
    }
    if ("json" === streamType) {
        return QUERY_JOB_TYPE.EXTRACT_JSON;
    }

    return null;
};

/**
 * Submits queries and renders the query states.
 *
 * @return {React.ReactElement}
 */
const QueryStatus = () => {
    const [queryState, setQueryState] = useState(QUERY_LOADING_STATES.SUBMITTING);
    const [errorMsg, setErrorMsg] = useState(null);
    const isFirstRun = useRef(true);

    useEffect(() => {
        if (false === isFirstRun.current) {
            return;
        }
        isFirstRun.current = false;

        const searchParams = new URLSearchParams(window.location.search);
        const streamType = searchParams.get("type");
        const targetId = searchParams.get("targetId");
        const logEventIdx = searchParams.get("logEventIdx");

        if (null === streamType || null === targetId || null === logEventIdx) {
            const error = "Queries parameters are missing from the URL " +
             "parameters.";

            console.error(error);
            setErrorMsg(error);

            return;
        }

        const extractJobType = getExtractJobType(streamType);
        if (null === extractJobType) {
            const error = `Unsupported Target type: ${streamType}`;
            console.error(error);
            setErrorMsg(error);

            return;
        }

        submitExtractStreamJob(
            extractJobType,
            targetId,
            Number(logEventIdx),
            () => {
                setQueryState(QUERY_LOADING_STATES.WAITING);
            }
        )
            .then(({data}) => {
                setQueryState(QUERY_LOADING_STATES.LOADING);

                const innerLogEventNum = logEventIdx - data.begin_msg_ix + 1;
                window.location = `/log-viewer/index.html?filePath=/stream/${data.path}` +
                    `#logEventNum=${innerLogEventNum}`;
            })
            .catch((e) => {
                let msg = "Unknown error.";
                if (e instanceof AxiosError) {
                    msg = e.message;
                    if ("undefined" !== typeof e.response) {
                        if ("undefined" !== typeof e.response.data.message) {
                            msg = e.response.data.message;
                        } else {
                            msg = e.response.statusText;
                        }
                    }
                }
                console.error(msg, e);
                setErrorMsg(msg);
            });
    }, []);

    return (
        <Loading
            currentState={queryState}
            errorMsg={errorMsg}/>
    );
};

export default QueryStatus;

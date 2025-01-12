import {
    useEffect,
    useRef,
    useState,
} from "react";

import {isAxiosError} from "axios";

import {submitExtractStreamJob} from "../api/query";
import {Nullable} from "../typings/common";
import {
    EXTRACT_JOB_TYPE,
    QUERY_LOADING_STATE,
} from "../typings/query";
import Loading from "./Loading";


/**
 * Submits queries and renders the query states.
 *
 * @return
 */
const QueryStatus = () => {
    const [queryState, setQueryState] = useState<QUERY_LOADING_STATE>(
        QUERY_LOADING_STATE.SUBMITTING
    );
    const [errorMsg, setErrorMsg] = useState<Nullable<string>>(null);
    const isFirstRun = useRef(true);

    useEffect(() => {
        if (false === isFirstRun.current) {
            return;
        }
        isFirstRun.current = false;

        const searchParams = new URLSearchParams(window.location.search);
        const streamType = searchParams.get("type");
        const streamId = searchParams.get("streamId");
        const logEventIdx = searchParams.get("logEventIdx");

        if (null === streamType || null === streamId || null === logEventIdx) {
            const error = "Queries parameters are missing from the URL parameters.";
            console.error(error);
            setErrorMsg(error);

            return;
        }

        if (false === streamType in EXTRACT_JOB_TYPE) {
            const error = `Unsupported Stream type: ${streamType}`;
            console.error(error);
            setErrorMsg(error);

            return;
        }
        const sanitizedLogEventIdx = Number(logEventIdx);
        const extractJobType = EXTRACT_JOB_TYPE[streamType as keyof typeof EXTRACT_JOB_TYPE];

        submitExtractStreamJob(
            extractJobType,
            streamId,
            sanitizedLogEventIdx,
            () => {
                setQueryState(QUERY_LOADING_STATE.WAITING);
            }
        )
            .then(({data}) => {
                setQueryState(QUERY_LOADING_STATE.LOADING);

                const innerLogEventNum = sanitizedLogEventIdx - data.begin_msg_ix + 1;
                window.location.href = `/log-viewer/index.html?filePath=/streams/${data.path}` +
                    `#logEventNum=${innerLogEventNum}`;
            })
            .catch((e: unknown) => {
                let msg = "Unknown error.";
                if (isAxiosError<{message?: string}>(e)) {
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

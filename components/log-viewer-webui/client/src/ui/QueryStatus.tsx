import {
    useEffect,
    useState,
} from "react";

import {Static} from "@sinclair/typebox";
import {
    AssertError,
    Value,
} from "@sinclair/typebox/value";
import {isAxiosError} from "axios";

import {submitExtractStreamJob} from "../api/query";
import {Nullable} from "../typings/common";
import {
    EXTRACT_JOB_TYPE,
    ExtractJobSearchParams,
    QUERY_LOADING_STATE,
} from "../typings/query";
import Loading from "./Loading";


/**
 * Flag to prevent duplicate execution of `useEffect`.
 */
let isFirstRun = true;

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

    useEffect(() => {
        // eslint-disable-next-line no-warning-comments
        // TODO: Address server-side concurrency issues and replace this workaround by aborting
        // requests via an AbortController in useEffect's cleanup function.

        // Skip duplicate execution caused by React StrictMode.
        if (false === isFirstRun) {
            return;
        }
        isFirstRun = false;

        // Validates and parse search parameters.
        const searchParams = new URLSearchParams(window.location.search);
        const paramsObj = Object.fromEntries(searchParams);
        let parseResult: Static<typeof ExtractJobSearchParams>;
        try {
            parseResult = Value.Parse(ExtractJobSearchParams, paramsObj);
        } catch (e: unknown) {
            let error = "URL parameters parsing failed";
            if (e instanceof AssertError) {
                error += `: ${e.message}`;
            }
            console.error(error);
            setErrorMsg(error);

            return;
        }

        submitExtractStreamJob(

            // `parseResult.type` must be valid key since parsed using with typebox type
            // `ExtractJobSearchParams`.
            EXTRACT_JOB_TYPE[parseResult.type as keyof typeof EXTRACT_JOB_TYPE],
            parseResult.streamId,
            parseResult.logEventIdx,
            () => {
                setQueryState(QUERY_LOADING_STATE.WAITING);
            }
        )
            .then(({data}) => {
                setQueryState(QUERY_LOADING_STATE.LOADING);

                const innerLogEventNum = parseResult.logEventIdx - data.begin_msg_ix + 1;
                window.location.href = `/log-viewer/index.html?filePath=${data.path}` +
                    `#logEventNum=${innerLogEventNum}`;
            })
            .catch((e: unknown) => {
                let msg = "Unknown error.";
                if (isAxiosError<{message: string}>(e)) {
                    msg = e.message;
                    if ("undefined" !== typeof e.response) {
                        msg = e.response.data.message;
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

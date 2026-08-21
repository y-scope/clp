import {
    useEffect,
    useState,
} from "react";

import {Static} from "@sinclair/typebox";
import {
    AssertError,
    Value,
} from "@sinclair/typebox/value";
import {Nullable} from "@webui/common/utility-types";

import {submitExtractStreamJob} from "../../api/stream-files";
import {
    EXTRACT_JOB_TYPE,
    ExtractJobSearchParams,
    QUERY_LOADING_STATE,
} from "../../typings/query";
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

        const {
            dataset,
            type,
            logEventIdx,
            streamId,
        } = parseResult;

        submitExtractStreamJob({
            dataset: "undefined" === typeof dataset ?
                null :
                dataset,

            // `parseResult.type` must be valid key since parsed using with typebox type
            // `ExtractJobSearchParams`.
            extractJobType: EXTRACT_JOB_TYPE[type as keyof typeof EXTRACT_JOB_TYPE],
            logEventIdx: logEventIdx,
            onRequestStarted: () => {
                setQueryState(QUERY_LOADING_STATE.WAITING);
            },
            streamId: streamId,
        })
            .then(({data}) => {
                setQueryState(QUERY_LOADING_STATE.LOADING);

                const innerLogEventNum = parseResult.logEventIdx - data.begin_msg_ix + 1;
                const filePath = encodeURIComponent(data.path);
                window.location.href =
                    `/log-viewer/index.html?filePath=${filePath}` +
                    `#logEventNum=${innerLogEventNum}`;
            })
            .catch((e: unknown) => {
                const msg = e instanceof Error ?
                    e.message :
                    "Unknown error.";

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

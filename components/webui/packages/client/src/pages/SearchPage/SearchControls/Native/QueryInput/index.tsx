import {
    ChangeEvent,
    useCallback,
    useEffect,
    useRef,
} from "react";

import type {InputRef} from "antd";

import QueryBox from "../../../../../components/QueryBox";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {usePseudoProgress} from "../../../SearchState/usePseudoProgress";


/**
 * Renders a query input and pseudo progress bar.
 *
 * @return
 */
const QueryInput = () => {
    const queryIsCaseSensitive = useSearchStore((state) => state.queryIsCaseSensitive);
    const queryString = useSearchStore((state) => state.queryString);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const {progress: pseudoProgress, start, stop} = usePseudoProgress();
    const inputRef = useRef<InputRef>(null);

    const handleCaseSensitiveChange = useCallback((newValue: boolean) => {
        const {updateQueryIsCaseSensitive} = useSearchStore.getState();
        updateQueryIsCaseSensitive(newValue);
    }, []);

    const handleChange = useCallback((ev: ChangeEvent<HTMLInputElement>) => {
        const {updateQueryString} = useSearchStore.getState();
        updateQueryString(ev.target.value);
    }, []);

    useEffect(() => {
        if (searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING) {
            start();
        } else if (
            searchUiState === SEARCH_UI_STATE.DONE ||
            searchUiState === SEARCH_UI_STATE.FAILED
        ) {
            stop();
        }
    }, [searchUiState,
        start,
        stop]);

    useEffect(() => {
        if (
            searchUiState === SEARCH_UI_STATE.DEFAULT ||
            searchUiState === SEARCH_UI_STATE.DONE ||
            searchUiState === SEARCH_UI_STATE.FAILED
        ) {
            inputRef.current?.focus();
        }
    }, [searchUiState]);

    return (
        <QueryBox
            isCaseSensitive={queryIsCaseSensitive}
            placeholder={"Enter your query"}
            progress={pseudoProgress}
            ref={inputRef}
            size={"middle"}
            value={queryString}
            disabled={
                searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                searchUiState === SEARCH_UI_STATE.QUERYING
            }
            onCaseSensitiveChange={handleCaseSensitiveChange}
            onChange={handleChange}/>
    );
};


export default QueryInput;

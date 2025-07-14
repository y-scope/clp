import {
    ChangeEvent,
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {Nullable} from "src/typings/common";

import QueryBox from "../../../../components/QueryBox";
import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {
    PROGRESS_INCREMENT,
    PROGRESS_INTERVAL_MILLIS,
} from "./typings";


/**
 * Renders a query input and pseudo progress bar.
 *
 * @return
 */
const QueryInput = () => {
    const queryIsCaseSensitive = useSearchStore((state) => state.queryIsCaseSensitive);
    const queryString = useSearchStore((state) => state.queryString);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const [pseudoProgress, setPseudoProgress] = useState<Nullable<number>>(null);
    const intervalIdRef = useRef<number>(0);

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
            if (0 !== intervalIdRef.current) {
                console.warn("Interval already set for submitted query");

                return;
            }
            intervalIdRef.current = window.setInterval(() => {
                setPseudoProgress((v) => {
                    if (100 <= (v ?? 0) + PROGRESS_INCREMENT) {
                        return 100;
                    }

                    return (v ?? 0) + PROGRESS_INCREMENT;
                });
            }, PROGRESS_INTERVAL_MILLIS);
        } else if (searchUiState === SEARCH_UI_STATE.DONE) {
            clearInterval(intervalIdRef.current);
            intervalIdRef.current = 0;
            setPseudoProgress(null);
        }
    }, [searchUiState]);

    // Clear the interval if the component unmounts.
    useEffect(() => {
        clearInterval(intervalIdRef.current);
    }, []);

    return (
        <QueryBox
            isCaseSensitive={queryIsCaseSensitive}
            placeholder={"Enter your query"}
            progress={pseudoProgress}
            size={"large"}
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

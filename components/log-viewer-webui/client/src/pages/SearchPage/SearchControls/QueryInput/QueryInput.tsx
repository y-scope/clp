import QueryBox from "../../../../components/QueryBox";
import useSearchStore from "../../SearchState/index";
import {
    useEffect,
    useRef,
    useState,
} from "react";
import { SEARCH_UI_STATE } from "../../SearchState/typings";
import { PROGRESS_INCREMENT, PROGRESS_INTERVAL_MILLIS } from "./typings";

/**
 * Renders a query input and pseudo progress bar.
 *
 * @return
 */
const QueryInput = () => {
    const {queryString, updateQueryString, searchUiState}= useSearchStore();
    const [pseudoProgress, setPseudoProgress] = useState<number>(0);
    const intervalIdRef = useRef<number>(0);

    useEffect(() => {
        if (searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING)
        {
            if (intervalIdRef.current !== 0) {
                console.warn("Interval already set for submitted query");
                return;
            }
            intervalIdRef.current = window.setInterval(() => {
                setPseudoProgress((v) => {
                    if (v + PROGRESS_INCREMENT >= 100) {
                        return 100;
                    }
                    return v + PROGRESS_INCREMENT;
                });
            }, PROGRESS_INTERVAL_MILLIS);
        }
        else if (searchUiState === SEARCH_UI_STATE.DONE) {
            clearInterval(intervalIdRef.current);
            intervalIdRef.current = 0;
            setPseudoProgress(0);
        }
    }, [searchUiState]);

    // Clear the interval if the component unmounts.
    useEffect(() => {
        clearInterval(intervalIdRef.current);
    }, []);

    return (
        <QueryBox
            placeholder={"Enter your query"}
            progress={pseudoProgress}
            size={"large"}
            disabled={
                searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                searchUiState === SEARCH_UI_STATE.QUERYING
            }
            value={queryString}
            onChange={(e) => {
                updateQueryString(e.target.value);
            }}/>
    );
};


export default QueryInput;

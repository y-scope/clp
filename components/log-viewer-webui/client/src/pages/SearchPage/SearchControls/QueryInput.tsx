import QueryBox from "../../../components/QueryBox";
import useSearchStore from "../SearchState/index";
import {
    useEffect,
    useRef,
    useState,
} from "react";
import { SEARCH_UI_STATE } from "../SearchState/typings";


/**
 * Pseudo progress bar increments on each interval tick.
 */
const PROGRESS_INCREMENT = 5;

/**
 * The interval for updating the pseudo progress bar.
 */
const PROGRESS_INTERVAL_MILLIS = 100;

/**
 * Renders controls for submitting queries.
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
                // This should not happen since it should only enter QUERY_ID_PENDING once,
                // but just in case, we exit to avoid multiple intervals.
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

    // Clear the interval when the component unmounts.
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

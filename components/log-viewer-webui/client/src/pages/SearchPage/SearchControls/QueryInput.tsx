import QueryBox from "../../../components/QueryBox";
import useSearchStore from "../SearchState/index";
import {
    useEffect,
    useRef,
    useState,
} from "react";
import { SEARCH_UI_STATE } from "../SearchState/typings";

// for pseudo progress bar
const PROGRESS_INCREMENT = 5;
const PROGRESS_INTERVAL_MILLIS = 100;

/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const QueryInput = () => {
    const {queryString, updateQueryString, searchUiState}= useSearchStore();
    const [progress, setProgress] = useState<number>(0);
    const timerIntervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

    useEffect(() => {
        if (searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
            searchUiState === SEARCH_UI_STATE.QUERYING)
        {
            if (null === timerIntervalRef.current) {
                timerIntervalRef.current = setInterval(() => {
                    setProgress((v) => {
                        if (v + PROGRESS_INCREMENT >= 100) {
                            return 100;
                        }
                        return v + PROGRESS_INCREMENT;
                    });
                }, PROGRESS_INTERVAL_MILLIS);
            }
        } else {
            if (null !== timerIntervalRef.current) {
                clearInterval(timerIntervalRef.current);
                timerIntervalRef.current = null;
            }
            setProgress(0);
        }
    }, [searchUiState]);

    return (
        <QueryBox
            placeholder={"Enter your query"}
            progress={progress}
            size={"large"}
            disabled={  searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                        searchUiState === SEARCH_UI_STATE.QUERYING
            }
            value={queryString}
            onChange={(e) => {
                updateQueryString(e.target.value);
            }}/>
    );
};


export default QueryInput;

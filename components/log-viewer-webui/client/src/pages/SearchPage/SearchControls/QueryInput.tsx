import QueryBox from "../../../components/QueryBox";
import useSearchStore from "../SearchState/index";
import { isSearchSignalQuerying, SearchResultsMetadataDocument } from "@common/searchResultsMetadata.js";
import { useResultsMetadata } from "../metadata";

import {
    useEffect,
    useRef,
    useState,
} from "react";

// for pseudo progress bar
const PROGRESS_INCREMENT = 5;
const PROGRESS_INTERVAL_MILLIS = 100;

/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const QueryInput = () => {
    const queryString = useSearchStore((state) => state.queryString);
    const updateQueryString = useSearchStore((state) => state.updateQueryString);
    const resultsMetadata = useResultsMetadata();
    const [progress, setProgress] = useState(0);
    const timerIntervalRef = useRef<NodeJS.Timeout>(null);

    console.log("QueryInput: resultsMetadata", resultsMetadata);

    useEffect(() => {
        // I have idea to potentially just set the searchSignal here
        // but will get back to later
        if (Array.isArray(resultsMetadata) && resultsMetadata.length === 0) {
            return;
        }
        let resultMetadata = resultsMetadata[0] as SearchResultsMetadataDocument;
        console.log(resultsMetadata);
        console.log("QueryInput: resultsMetadata.lastSignal", resultMetadata.lastSignal);
        if (true === isSearchSignalQuerying(resultMetadata.lastSignal)) {
            console.log("QueryInput: isSearchSignalQuerying");
            if (null === timerIntervalRef.current) {
                timerIntervalRef.current = setInterval(() => {
                    setProgress((v) => {
                        if (v + PROGRESS_INCREMENT >= 100) {
                            clearInterval(timerIntervalRef.current!);
                            timerIntervalRef.current = null;
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
    }, [resultsMetadata]);

    return (
        <QueryBox
            placeholder={"Enter your query"}
            progress={progress}
            size={"large"}
            value={queryString}
            onChange={(e) => {
                updateQueryString(e.target.value);
            }}/>
    );
};


export default QueryInput;

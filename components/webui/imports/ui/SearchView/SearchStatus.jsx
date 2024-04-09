import {
    useEffect,
    useRef,
    useState,
} from "react";
import ProgressBar from "react-bootstrap/ProgressBar";

import {faExclamationCircle} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {
    isSearchSignalQuerying, SEARCH_SIGNAL,
} from "/imports/api/search/constants";


// for pseudo progress bar
const PROGRESS_INCREMENT = 5;
const PROGRESS_INTERVAL_MS = 100;

/**
 * Displays the status of a search operation, which shows error messages if any, and otherwise
 * displays the current status of the search.
 *
 * @param {object} props
 * @param {SearchResultsMetadata} props.resultsMetadata including the last search signal
 * @param {string} props.errorMsg if there is an error
 * @return {React.ReactElement}
 */
const SearchStatus = ({
    resultsMetadata,
    errorMsg,
}) => {
    const [progress, setProgress] = useState(0);
    const timerIntervalRef = useRef(null);

    useEffect(() => {
        if (true === isSearchSignalQuerying(resultsMetadata.lastSignal)) {
            timerIntervalRef.current ??= setInterval(() => {
                setProgress((v) => (v + PROGRESS_INCREMENT));
            }, PROGRESS_INTERVAL_MS);
        } else {
            if (null !== timerIntervalRef.current) {
                clearInterval(timerIntervalRef.current);
                timerIntervalRef.current = null;
            }
            setProgress(0);
        }
    }, [resultsMetadata.lastSignal]);

    if ("" !== errorMsg && null !== errorMsg && "undefined" !== typeof errorMsg) {
        return (
            <div className={"search-error"}>
                <FontAwesomeIcon
                    className={"search-error-icon"}
                    icon={faExclamationCircle}/>
                {errorMsg}
            </div>
        );
    }
    let message = null;
    switch (resultsMetadata.lastSignal) {
        case SEARCH_SIGNAL.NONE:
            message = "Ready";
            break;
        case SEARCH_SIGNAL.REQ_CLEARING:
            message = "Clearing...";
            break;
        default:
            break;
    }

    return (
        <>
            <ProgressBar
                animated={true}
                className={"search-progress-bar rounded-0 border-bottom"}
                now={progress}
                striped={true}
                variant={"primary"}
                style={{visibility: (0 === progress) ?
                    "hidden" :
                    "visible"}}/>
            {(null !== message) &&
            <div className={"search-no-results-status"}>
                {message}
            </div>}
        </>
    );
};

export default SearchStatus;

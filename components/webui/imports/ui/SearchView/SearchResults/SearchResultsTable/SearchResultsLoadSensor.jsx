import {
    useEffect, useRef,
} from "react";
import Spinner from "react-bootstrap/Spinner";

import {faCircleInfo} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {SEARCH_MAX_NUM_RESULTS} from "../../../../api/search/constants";

import "./SearchResultsLoadSensor.scss";


/**
 * The interval, in milliseconds, at which the search results load sensor should poll for updates.
 */
const SEARCH_RESULTS_LOAD_SENSOR_POLL_INTERVAL_MILLIS = 200;

/**
 * Senses if the user has requested to load more results by scrolling until
 * this element becomes partially visible.
 *
 * @param {object} props
 * @param {boolean} props.hasMoreResultsInCache
 * @param {boolean} props.hasMoreResultsInTotal
 * @param {Function} props.onLoadMoreResults
 * @return {React.ReactElement}
 */
const SearchResultsLoadSensor = ({
    hasMoreResultsInCache,
    hasMoreResultsInTotal,
    onLoadMoreResults,
}) => {
    const loadingBlockRef = useRef(null);
    const loadIntervalRef = useRef(null);

    useEffect(() => {
        if (false === hasMoreResultsInCache) {
            return () => null;
        }

        const observer = new IntersectionObserver((entries) => {
            if (entries[0].isIntersecting) {
                onLoadMoreResults();
                loadIntervalRef.current = setInterval(
                    onLoadMoreResults,
                    SEARCH_RESULTS_LOAD_SENSOR_POLL_INTERVAL_MILLIS,
                );
            } else if (null !== loadIntervalRef.current) {
                clearInterval(loadIntervalRef.current);
                loadIntervalRef.current = null;
            }
        });

        observer.observe(loadingBlockRef.current);

        return () => {
            if (null !== loadIntervalRef.current) {
                clearInterval(loadIntervalRef.current);
                loadIntervalRef.current = null;
            }
            observer.disconnect();
        };
    }, [
        hasMoreResultsInCache,
        onLoadMoreResults,
    ]);

    return (
        <div
            ref={loadingBlockRef}
            style={{
                visibility: (true === hasMoreResultsInCache || true === hasMoreResultsInTotal) ?
                    "visible" :
                    "hidden",
            }}
        >
            {(true === hasMoreResultsInCache) &&
                <div className={"search-results-load-sensor-content"}>
                    <Spinner
                        animation={"border"}
                        size={"sm"}
                        variant={"primary"}/>
                    <span>Loading...</span>
                </div>}
            {(false === hasMoreResultsInCache && true === hasMoreResultsInTotal) &&
                <div className={"search-results-load-sensor-content"}>
                    <FontAwesomeIcon
                        icon={faCircleInfo}
                        size={"sm"}/>
                    <span>
                        Results limit
                        {" "}
                        {SEARCH_MAX_NUM_RESULTS}
                        {" "}
                        reached. Please refine the filters to display additional matches.
                    </span>
                </div>}
        </div>
    );
};

export default SearchResultsLoadSensor;

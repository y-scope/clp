import {
    useEffect, useRef,
} from "react";
import Spinner from "react-bootstrap/Spinner";

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
 * @param {boolean} props.hasMoreResults
 * @param {Function} props.onLoadMoreResults
 * @return {React.ReactElement}
 */
const SearchResultsLoadSensor = ({
    hasMoreResults,
    onLoadMoreResults,
}) => {
    const loadingBlockRef = useRef(null);
    const loadIntervalRef = useRef(null);

    useEffect(() => {
        if (false === hasMoreResults) {
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
        hasMoreResults,
        onLoadMoreResults,
    ]);

    return (
        <div
            id={"search-results-load-sensor"}
            ref={loadingBlockRef}
            style={{
                visibility: (true === hasMoreResults) ?
                    "visible" :
                    "hidden",
            }}
        >
            <Spinner
                animation={"border"}
                size={"sm"}
                variant={"primary"}/>
            <span>Loading...</span>
        </div>
    );
};

export default SearchResultsLoadSensor;

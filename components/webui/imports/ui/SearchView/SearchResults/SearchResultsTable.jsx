import {
    useEffect,
    useRef,
} from "react";
import {
    Spinner, Table,
} from "react-bootstrap";

import {
    faSort,
    faSortDown,
    faSortUp,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {
    MONGO_SORT_ORDER,
    SEARCH_RESULTS_FIELDS,
} from "/imports/api/search/constants";

import "./SearchResultsTable.scss";


/**
 * The interval, in milliseconds, at which the search results load sensor should poll for updates.
 */
const SEARCH_RESULTS_LOAD_SENSOR_POLL_INTERVAL_MS = 200;

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
                    SEARCH_RESULTS_LOAD_SENSOR_POLL_INTERVAL_MS,
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

/**
 * Represents a table component to display search results.
 *
 * @param {object} props
 * @param {object} props.fieldToSortBy
 * @param {boolean} props.hasMoreResults
 * @param {number} props.maxLinesPerResult
 * @param {Function} props.onLoadMoreResults
 * @param {object} props.searchResults
 * @param {Function} props.setFieldToSortBy
 * @return {React.ReactElement}
 */
const SearchResultsTable = ({
    fieldToSortBy,
    hasMoreResults,
    maxLinesPerResult,
    onLoadMoreResults,
    searchResults,
    setFieldToSortBy,
}) => {
    const getSortIcon = (fieldName) => {
        if (fieldName === fieldToSortBy.name) {
            return (MONGO_SORT_ORDER.ASCENDING === fieldToSortBy.direction) ?
                faSortUp :
                faSortDown;
        }

        return faSort;
    };

    const toggleSortDirection = (event) => {
        const {columnName} = event.currentTarget.dataset;
        if (fieldToSortBy.name !== columnName) {
            setFieldToSortBy({
                name: columnName,
                direction: (SEARCH_RESULTS_FIELDS.TIMESTAMP === columnName) ?
                    MONGO_SORT_ORDER.DESCENDING :
                    MONGO_SORT_ORDER.ASCENDING,
            });
        } else {
            setFieldToSortBy({
                name: columnName,
                direction: (MONGO_SORT_ORDER.ASCENDING === fieldToSortBy.direction) ?
                    MONGO_SORT_ORDER.DESCENDING :
                    MONGO_SORT_ORDER.ASCENDING,
            });
        }
    };

    const rows = [];

    // Construct rows
    for (let i = 0; i < searchResults.length; ++i) {
        const searchResult = searchResults[i];
        rows.push(<tr key={searchResult._id}>
            <td>
                {searchResult.timestamp ?
                    new Date(searchResult.timestamp).toISOString()
                        .slice(0, 19)
                        .replace("T", " ") :
                    "N/A"}
            </td>
            <td>
                <pre
                    className={"search-results-message"}
                    style={{maxHeight: `${maxLinesPerResult * 1.4}rem`}}
                >
                    {searchResult.message}
                </pre>
            </td>
        </tr>);
    }

    return (
        <div className={"search-results-container"}>
            <Table
                className={"border-bottom search-results"}
                hover={true}
                responsive={"sm"}
                striped={true}
            >
                <thead>
                    <tr>
                        <th
                            className={"search-results-th search-results-th-sortable"}
                            data-column-name={SEARCH_RESULTS_FIELDS.TIMESTAMP}
                            key={SEARCH_RESULTS_FIELDS.TIMESTAMP}
                            style={{width: "144px"}}
                            onClick={toggleSortDirection}
                        >
                            <div className={"search-results-table-header"}>
                                <FontAwesomeIcon
                                    icon={getSortIcon(SEARCH_RESULTS_FIELDS.TIMESTAMP)}/>
                                <span> Timestamp</span>
                            </div>
                        </th>
                        <th
                            className={"search-results-th"}
                            key={"message"}
                        >
                            <div className={"search-results-table-header"}>
                                Log message
                            </div>
                        </th>
                    </tr>
                </thead>
                <tbody>
                    {rows}
                </tbody>
            </Table>
            <SearchResultsLoadSensor
                hasMoreResults={hasMoreResults}
                onLoadMoreResults={onLoadMoreResults}/>
        </div>
    );
};

export default SearchResultsTable;

import React, {useEffect, useRef} from "react";

import {faSort, faSortDown, faSortUp} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {Spinner, Table} from "react-bootstrap";

import "./SearchResultsTable.scss";
import {
    MONGO_SORT_ORDER,
    SEARCH_RESULTS_FIELDS,
} from "../../api/search/constants";


/**
 * The interval, in milliseconds, at which the search results load sensor should poll for updates.
 */
const SEARCH_RESULTS_LOAD_SENSOR_POLL_INTERVAL_MS = 200;

/**
 * Senses if the user has requested to load more results by scrolling until
 * this element becomes partially visible.
 *
 * @param {boolean} hasMoreResults
 * @param {function} onLoadMoreResults
 * @returns {JSX.Element}
 */
const SearchResultsLoadSensor = ({
    hasMoreResults,
    onLoadMoreResults,
}) => {
    const loadingBlockRef = useRef(null);
    const loadIntervalRef = useRef(null);

    useEffect(() => {
        const observer = new IntersectionObserver((entries) => {
            if (entries[0].isIntersecting && false === hasMoreResults) {
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

    return <div
        id={"search-results-load-sensor"}
        ref={loadingBlockRef}
        style={{
            visibility: (true === hasMoreResults) ?
                "visible" :
                "hidden",
        }}
    >
        <Spinner animation="border" variant="primary" size="sm"/>
        <span>Loading...</span>
    </div>;
};

/**
 * Represents a table component to display search results.
 *
 * @param {Object} searchResults
 * @param {number} maxLinesPerResult
 * @param {Object} fieldToSortBy
 * @param {function} setFieldToSortBy
 * @param {boolean} hasMoreResults
 * @param {function} onLoadMoreResults
 * @returns {JSX.Element}
 */
const SearchResultsTable = ({
    searchResults,
    maxLinesPerResult,
    fieldToSortBy,
    setFieldToSortBy,
    hasMoreResults,
    onLoadMoreResults,
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

    let rows = [];

    // Construct rows
    for (let i = 0; i < searchResults.length; ++i) {
        let searchResult = searchResults[i];
        rows.push(<tr key={searchResult._id}>
            <td>{searchResult.timestamp ? new Date(searchResult.timestamp).toISOString().
                slice(0, 19).
                replace("T", " ") : "N/A"}</td>
            <td>
                <pre className="search-results-message"
                     style={{maxHeight: (maxLinesPerResult * 1.4) + "rem"}}>
                    {searchResult.message}
                </pre>
            </td>
        </tr>);
    }

    return (<div className={"search-results-container"}>
        <Table striped hover responsive="sm" className={"border-bottom search-results"}>
            <thead>
            <tr>
                <th style={{"width": "144px"}}
                    className={"search-results-th search-results-th-sortable"}
                    data-column-name={SEARCH_RESULTS_FIELDS.TIMESTAMP}
                    key={SEARCH_RESULTS_FIELDS.TIMESTAMP}
                    onClick={toggleSortDirection}
                >
                    <div className={"search-results-table-header"}>
                        <FontAwesomeIcon
                            icon={getSortIcon(SEARCH_RESULTS_FIELDS.TIMESTAMP)}/>
                        <span> Timestamp</span>
                    </div>
                </th>
                <th className={"search-results-th"} key={"message"}>
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
    </div>);
};

export default SearchResultsTable;

import {faSort, faSortDown, faSortUp} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import React, {useEffect, useState} from "react";
import {Spinner, Table} from "react-bootstrap";
import ReactVisibilitySensor from "react-visibility-sensor";

export const VISIBLE_RESULTS_LIMIT_INITIAL = 10;
const VISIBLE_RESULTS_LIMIT_INCREMENT = 10;

/**
 * Renders a table displaying search results, which includes features like sorting and dynamic
 * loading of more results when scrolling to the bottom, up to the number of results available on
 * the server.
 *
 * @param {Object[]} searchResults results to display
 * @param {number} maxLinesPerResult maximum number of lines to display per search result
 * @param {Object} fieldToSortBy used for sorting results
 * @param {function} setFieldToSortBy callback to set fieldToSortBy
 * @param {number} numResultsOnServer total number of results available on the server
 * @param {number} visibleSearchResultsLimit limit of visible search results
 * @param {function} setVisibleSearchResultsLimit callback to set visibleSearchResultsLimit
 * @returns {JSX.Element}
 */
export const SearchResultsTable = ({
    searchResults,
    maxLinesPerResult,
    fieldToSortBy,
    setFieldToSortBy,
    numResultsOnServer,
    visibleSearchResultsLimit,
    setVisibleSearchResultsLimit,
}) => {
    const [visibilitySensorVisible, setVisibilitySensorVisible] = useState(false);

    useEffect(() => {
        if (true === visibilitySensorVisible && visibleSearchResultsLimit <= numResultsOnServer) {
            setVisibleSearchResultsLimit(
                visibleSearchResultsLimit + VISIBLE_RESULTS_LIMIT_INCREMENT);
        }
    }, [visibilitySensorVisible, numResultsOnServer]);

    const getSortIcon = (fieldToSortBy, fieldName) => {
        if (fieldToSortBy && fieldName === fieldToSortBy.name) {
            return (1 === fieldToSortBy.direction ? faSortDown : faSortUp);
        } else {
            return faSort;
        }
    };

    const toggleSortDirection = (event) => {
        const columnName = event.currentTarget.dataset.columnName;
        if (null === fieldToSortBy || fieldToSortBy.name !== columnName) {
            setFieldToSortBy({
                name: columnName,
                direction: 1,
            });
        } else if (1 === fieldToSortBy.direction) {
            // Switch to descending
            setFieldToSortBy({
                name: columnName,
                direction: -1,
            });
        } else if (-1 === fieldToSortBy.direction) {
            // Switch to unsorted
            setFieldToSortBy(null);
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
                    data-column-name={"timestamp"}
                    key={"timestamp"}
                    onClick={toggleSortDirection}
                >
                    <div className={"search-results-table-header"}>
                        <FontAwesomeIcon icon={getSortIcon(fieldToSortBy, "timestamp")}/> Timestamp
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
        <ReactVisibilitySensor
            onChange={setVisibilitySensorVisible}
            scrollCheck={true}
            scrollDelay={200}
            intervalCheck={true}
            intervalDelay={200}
            partialVisibility={true}
            resizeCheck={true}
        >
            <div style={{
                fontSize: "1.4em",
                visibility: visibilitySensorVisible &&
                (visibleSearchResultsLimit <= numResultsOnServer) ? "visible" : "hidden",
            }}>
                <Spinner animation="border" variant="primary" size="sm" style={{margin: "5px"}}/>
                <span>Loading</span>
            </div>
        </ReactVisibilitySensor>
    </div>);
};
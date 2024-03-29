import React, {useCallback} from "react";

import SearchResultsHeader from "./SearchResultsHeader.jsx";
import SearchResultsTable from "./SearchResultsTable.jsx";


/**
 * The initial visible results limit.
 *
 * @type {number}
 * @constant
 */
const VISIBLE_RESULTS_LIMIT_INITIAL = 10;

/**
 * The increment value for the visible results limit.
 *
 * @type {number}
 * @constant
 */
const VISIBLE_RESULTS_LIMIT_INCREMENT = 10;

/**
 * Renders the search results, which includes the search results header and the search results
 * table.
 *
 * @param {number} jobId of the search job
 * @param {Object[]} searchResults
 * @param {number} numResultsOnServer
 * @param {Object} fieldToSortBy
 * @param {function} setFieldToSortBy
 * @param {number} visibleSearchResultsLimit
 * @param {function} setVisibleSearchResultsLimit
 * @param {number} maxLinesPerResult
 * @param {function} setMaxLinesPerResult
 * @returns {JSX.Element}
 */
const SearchResults = ({
    jobId,
    numResultsOnServer,
    searchResults,
    fieldToSortBy,
    setFieldToSortBy,
    visibleSearchResultsLimit,
    setVisibleSearchResultsLimit,
    maxLinesPerResult,
    setMaxLinesPerResult,
}) => {
    const hasMoreResults = visibleSearchResultsLimit < numResultsOnServer;

    const handleLoadMoreResults = useCallback(() => {
        if (hasMoreResults) {
            setVisibleSearchResultsLimit(
                visibleSearchResultsLimit + VISIBLE_RESULTS_LIMIT_INCREMENT
            );
        }
    }, [
        hasMoreResults,
        visibleSearchResultsLimit,
    ]);

    return <>
        <div className={"flex-column"}>
            <SearchResultsHeader
                jobId={jobId}
                numResultsOnServer={numResultsOnServer}
                maxLinesPerResult={maxLinesPerResult}
                setMaxLinesPerResult={setMaxLinesPerResult}
            />
        </div>
        {(0 < searchResults.length) && <div className="flex-column overflow-auto">
            <SearchResultsTable
                searchResults={searchResults}
                maxLinesPerResult={maxLinesPerResult}
                setMaxLinesPerResult={setMaxLinesPerResult}
                fieldToSortBy={fieldToSortBy}
                setFieldToSortBy={setFieldToSortBy}
                hasMoreResults={hasMoreResults}
                onLoadMoreResults={handleLoadMoreResults}
            />
        </div>}
    </>;
};

export default SearchResults;
export {VISIBLE_RESULTS_LIMIT_INITIAL};

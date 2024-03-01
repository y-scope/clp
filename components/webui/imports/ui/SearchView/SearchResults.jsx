import React from "react";

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
 * @param {Object[]} searchResults results to display
 * @param {Object} resultsMetadata which includes total results count and last request / response signal
 * @param {Object} fieldToSortBy used for sorting results
 * @param {function} setFieldToSortBy callback to set fieldToSortBy
 * @param {number} visibleSearchResultsLimit limit of visible search results
 * @param {function} setVisibleSearchResultsLimit callback to set visibleSearchResultsLimit
 * @param {number} maxLinesPerResult to display
 * @param {function} setMaxLinesPerResult callback to set maxLinesPerResult
 * @returns {JSX.Element}
 */
const SearchResults = ({
    jobId,
    searchResults,
    resultsMetadata,
    fieldToSortBy,
    setFieldToSortBy,
    visibleSearchResultsLimit,
    setVisibleSearchResultsLimit,
    maxLinesPerResult,
    setMaxLinesPerResult,
}) => {
    const numResultsOnServer = resultsMetadata["numTotalResults"] || searchResults.length;
    const hasMoreResults = visibleSearchResultsLimit < numResultsOnServer;

    const handleLoadMoreResults = () => {
        setVisibleSearchResultsLimit((v) =>
            (v + VISIBLE_RESULTS_LIMIT_INCREMENT));
    };

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

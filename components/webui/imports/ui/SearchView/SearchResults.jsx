import React from "react";

import SearchResultsHeader from "./SearchResultsHeader.jsx";
import {SearchResultsTable} from "./SearchResultsTable.jsx";


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

    return <>
        <div className={"flex-column"}>
            <SearchResultsHeader
                jobId={jobId}
                resultsMetadata={resultsMetadata}
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
                numResultsOnServer={numResultsOnServer}
                visibleSearchResultsLimit={visibleSearchResultsLimit}
                setVisibleSearchResultsLimit={setVisibleSearchResultsLimit}
            />
        </div>}
    </>;
};

export default SearchResults;

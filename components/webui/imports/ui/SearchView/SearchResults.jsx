import React from "react";
import {SearchResultsHeader} from "./SearchResultsHeader";
import {SearchResultsTable} from "./SearchResultsTable";

export const SearchResults = ({
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
    const isMessageTable = searchResults.length === 0 ||
        Object.keys(searchResults[0]).includes("timestamp");
    return <>
        {isMessageTable && <div className={"flex-column"}>
            <SearchResultsHeader
                jobId={jobId}
                resultsMetadata={resultsMetadata}
                numResultsOnServer={numResultsOnServer}
                maxLinesPerResult={maxLinesPerResult}
                setMaxLinesPerResult={setMaxLinesPerResult}
            />
        </div>}
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

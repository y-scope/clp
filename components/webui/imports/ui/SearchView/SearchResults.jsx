import React, {useCallback} from "react";
import Container from "react-bootstrap/Container";
import Row from "react-bootstrap/Row";

import SearchResultsHeader from "./SearchResultsHeader.jsx";
import SearchResultsTable from "./SearchResultsTable.jsx";
import SearchResultsTimeline from "./SearchResultsTimeline.jsx";


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
 * @param {number} searchJobId of the search job
 * @param {Object[]} searchResults
 * @param {number} numResultsOnServer
 * @param {Object} fieldToSortBy
 * @param {function} setFieldToSortBy
 * @param {number} visibleSearchResultsLimit
 * @param {function} setVisibleSearchResultsLimit
 * @param {number} maxLinesPerResult
 * @param {function} setMaxLinesPerResult
 * @param {TimelineConfig} timelineConfig
 * @param {TimelineBucket[]} timelineBuckets
 * @param {function} onTimelineZoom
 * @returns {JSX.Element}
 */
const SearchResults = ({
    searchJobId,
    numResultsOnServer,
    searchResults,
    fieldToSortBy,
    setFieldToSortBy,
    visibleSearchResultsLimit,
    setVisibleSearchResultsLimit,
    maxLinesPerResult,
    setMaxLinesPerResult,
    timelineConfig,
    timelineBuckets,
    onTimelineZoom,
}) => {
    const hasMoreResults = visibleSearchResultsLimit < numResultsOnServer;

    const handleLoadMoreResults = useCallback(() => {
        setVisibleSearchResultsLimit(
            (v) => (v + VISIBLE_RESULTS_LIMIT_INCREMENT)
        );
    }, []);

    return (
        <>
            <Container
                className={"py-2"}
                fluid={true}
            >
                <Row>
                    <SearchResultsHeader
                        maxLinesPerResult={maxLinesPerResult}
                        numResultsOnServer={numResultsOnServer}
                        searchJobId={searchJobId}
                        setMaxLinesPerResult={setMaxLinesPerResult}/>
                </Row>
                <Row>
                    <SearchResultsTimeline
                        timelineBuckets={timelineBuckets}
                        timelineConfig={timelineConfig}
                        onTimelineZoom={onTimelineZoom}/>
                </Row>
            </Container>
            {0 < searchResults.length &&
                <SearchResultsTable
                    fieldToSortBy={fieldToSortBy}
                    hasMoreResults={hasMoreResults}
                    maxLinesPerResult={maxLinesPerResult}
                    searchResults={searchResults}
                    setFieldToSortBy={setFieldToSortBy}
                    setMaxLinesPerResult={setMaxLinesPerResult}
                    onLoadMoreResults={handleLoadMoreResults}/>}
        </>
    );
};

export default SearchResults;
export {VISIBLE_RESULTS_LIMIT_INITIAL};

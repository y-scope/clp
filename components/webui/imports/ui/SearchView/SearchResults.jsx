import {useCallback} from "react";
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
 * @param {Object} fieldToSortBy
 * @param {number} maxLinesPerResult
 * @param {number} numResultsOnServer
 * @param {function} onTimelineZoom
 * @param {object} resultsMetadata
 * @param {number} searchJobId of the search job
 * @param {Object[]} searchResults
 * @param {function} setFieldToSortBy
 * @param {function} setMaxLinesPerResult
 * @param {function} setVisibleSearchResultsLimit
 * @param {TimelineBucket[]} timelineBuckets
 * @param {TimelineConfig} timelineConfig
 * @param {number} visibleSearchResultsLimit
 * @returns {JSX.Element}
 */
const SearchResults = ({
    fieldToSortBy,
    maxLinesPerResult,
    numResultsOnServer,
    onTimelineZoom,
    resultsMetadata,
    searchJobId,
    searchResults,
    setFieldToSortBy,
    setMaxLinesPerResult,
    setVisibleSearchResultsLimit,
    timelineBuckets,
    timelineConfig,
    visibleSearchResultsLimit,
}) => {
    const hasMoreResults = visibleSearchResultsLimit < numResultsOnServer;

    const handleLoadMoreResults = useCallback(() => {
        if (hasMoreResults) {
            setVisibleSearchResultsLimit((v) => v + VISIBLE_RESULTS_LIMIT_INCREMENT);
        }
    }, [
        hasMoreResults,
        setVisibleSearchResultsLimit,
    ]);

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
                        resultsMetadata={resultsMetadata}
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

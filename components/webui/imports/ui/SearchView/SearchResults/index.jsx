import {useCallback} from "react";
import Container from "react-bootstrap/Container";
import Row from "react-bootstrap/Row";

import SearchResultsHeader from "./SearchResultsHeader";
import SearchResultsTable from "./SearchResultsTable";
import SearchResultsTimeline from "./SearchResultsTimeline";


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
 * @param {object} props
 * @param {number} props.estimatedNumResults
 * @param {object} props.fieldToSortBy
 * @param {number} props.maxLinesPerResult
 * @param {Function} props.onTimelineZoom
 * @param {SearchResultsMetadata} props.resultsMetadata
 * @param {number} props.searchJobId
 * @param {object[]} props.searchResults
 * @param {Function} props.setFieldToSortBy
 * @param {Function} props.setMaxLinesPerResult
 * @param {Function} props.setVisibleSearchResultsLimit
 * @param {TimelineBucket[]} props.timelineBuckets
 * @param {TimelineConfig} props.timelineConfig
 * @param {number} props.visibleSearchResultsLimit
 * @return {React.ReactElement}
 */
const SearchResults = ({
    estimatedNumResults,
    fieldToSortBy,
    maxLinesPerResult,
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
    let aggregatedCount = null;
    if (null !== timelineBuckets && 0 !== timelineBuckets.length) {
        aggregatedCount = timelineBuckets.reduce(
            (accumulator, currentValue) => (accumulator + currentValue.count),
            0,
        );
    }

    // The number of result in the results cache is available in different variables at different
    // times:
    // - when the search job ends, it will be in resultsMetadata.numTotalResults.
    // - while the query is in progress, it will be in estimatedNumResults.
    // - when the query starts, the other two variables will be null, so searchResults.length is the
    //   best estimate.
    const numResultsInCache =
        resultsMetadata.numTotalResults ||
        estimatedNumResults ||
        searchResults.length;

    const numResultsInTotal = aggregatedCount || numResultsInCache;

    const hasMoreResultsInCache = visibleSearchResultsLimit < numResultsInCache;
    const hasMoreResultsInTotal = visibleSearchResultsLimit < numResultsInTotal;

    const handleLoadMoreResults = useCallback(() => {
        if (hasMoreResultsInCache) {
            setVisibleSearchResultsLimit((v) => v + VISIBLE_RESULTS_LIMIT_INCREMENT);
        }
    }, [
        hasMoreResultsInCache,
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
                        numResultsInTotal={numResultsInTotal}
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
                    hasMoreResultsInCache={hasMoreResultsInCache}
                    hasMoreResultsInTotal={hasMoreResultsInTotal}
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

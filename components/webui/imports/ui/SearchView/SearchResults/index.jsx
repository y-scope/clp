import {useCallback} from "react";
import Container from "react-bootstrap/Container";
import Row from "react-bootstrap/Row";

import SearchResultsHeader from "./SearchResultsTable/SearchResultsHeader";
import SearchResultsTable from "./SearchResultsTable/SearchResultsTable";
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
 * @param {object} props.fieldToSortBy
 * @param {number} props.maxLinesPerResult
 * @param {number} props.numResultsOnServer
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

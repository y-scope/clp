import Container from "react-bootstrap/Container";

import SearchControlsCaseSensitivity from "./SearchControlsCaseSensitivity";
import SearchControlsTimeRangeInput from "./SearchControlsTimeRangeInput";

import "./SearchControlsFilterDrawer.scss";


/**
 * Renders the controls for filtering search results by time range, including a date picker and
 * preset time range options.
 *
 * @param {object} props
 * @param {boolean} props.ignoreCase
 * @param {Function} props.setIgnoreCase
 * @param {Function} props.setTimeRange
 * @param {TimeRange} props.timeRange
 * @return {React.ReactElement}
 */
const SearchControlsFilterDrawer = ({
    ignoreCase,
    setIgnoreCase,
    setTimeRange,
    timeRange,
}) => {
    return (
        <Container
            className={"search-filter-controls-drawer px-3 py-3 border-bottom"}
            fluid={true}
        >
            <SearchControlsTimeRangeInput
                setTimeRange={setTimeRange}
                timeRange={timeRange}/>
            <SearchControlsCaseSensitivity
                ignoreCase={ignoreCase}
                setIgnoreCase={setIgnoreCase}/>
        </Container>
    );
};

export default SearchControlsFilterDrawer;

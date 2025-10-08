import {Card} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import PrestoResultsTimeline from "./Presto/PrestoResultsTimeline";
import ResultsTimeline from "./ResultsTimeline";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    return (
        <Card>
            {CLP_QUERY_ENGINES.PRESTO === SETTINGS_QUERY_ENGINE ?
                <PrestoResultsTimeline/> :
                <ResultsTimeline/>}
        </Card>
    );
};

export default SearchResultsTimeline;

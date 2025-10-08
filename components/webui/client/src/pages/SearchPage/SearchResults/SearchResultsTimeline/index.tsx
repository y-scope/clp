import {Card} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import ClpResultsTimeline from "./ClpResultsTimeline";
import PrestoResultsTimeline from "./Presto/PrestoResultsTimeline";


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
                <ClpResultsTimeline/>}
        </Card>
    );
};

export default SearchResultsTimeline;

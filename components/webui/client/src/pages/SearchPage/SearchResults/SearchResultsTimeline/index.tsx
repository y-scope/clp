import {Card} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import PrestoResultsTimeline from "./Presto/PrestoResultsTimeline";
import ClpResultsTimeline from "./ClpResultsTimeline";


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

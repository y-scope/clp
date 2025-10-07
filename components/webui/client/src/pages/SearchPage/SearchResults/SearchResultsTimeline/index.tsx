import {Card} from "antd";

import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../../config";
import PrestoVirtualResultsTimeline from "./Presto/PrestoVirtualResultsTimeline";
import VirtualResultsTimeline from "./VirtualResultsTimeline";


/**
 * Renders timeline visualization of search results.
 *
 * @return
 */
const SearchResultsTimeline = () => {
    return (
        <Card>
            {CLP_QUERY_ENGINES.PRESTO === SETTINGS_QUERY_ENGINE ?
                <PrestoVirtualResultsTimeline/> :
                <VirtualResultsTimeline/>}
        </Card>
    );
};

export default SearchResultsTimeline;

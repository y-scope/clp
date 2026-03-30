import {CLP_QUERY_ENGINES} from "@webui/common/config";
import {Card} from "antd";

import {SETTINGS_QUERY_ENGINE} from "../../../../config";
import NativeResultsTimeline from "./Native/NativeResultsTimeline";
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
                <NativeResultsTimeline/>}
        </Card>
    );
};

export default SearchResultsTimeline;

import { SearchOutlined } from "@ant-design/icons";
import { Button, Tooltip } from "antd";
import { handleQuerySubmit} from "../requests";
import useSearchStore, { SEARCH_STATE_DEFAULT } from "../../SearchState/index";
import styles from "./index.module.css";
import { SEARCH_UI_STATE } from "../../SearchState/typings";
import { computeTimelineConfig } from "../../SearchResults/SearchResultsTimeline/utils";

const SubmitButton = () => {
    const {searchUiState, timeRange, queryString} = useSearchStore();
    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

    return (
        <Tooltip title={isQueryStringEmpty ? "Enter query to search" : ""}>
            <Button
                className={styles["gradientButton"] || ""}
                disabled={isQueryStringEmpty || searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                icon={<SearchOutlined />}
                size="large"
                type="primary"
                onClick={() => {
                    const timelineConfig = computeTimelineConfig(timeRange[0].valueOf(), timeRange[1].valueOf());

                    handleQuerySubmit({
                        queryString: queryString,
                        timestampBegin: timeRange[0].valueOf(),
                        timestampEnd: timeRange[1].valueOf(),
                        timeRangeBucketSizeMillis: timelineConfig.bucketDuration.asMilliseconds(),
                        ignoreCase: false,
                    });
                }}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;

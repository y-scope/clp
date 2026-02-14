import {useCallback} from "react";

import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    message,
    Tooltip,
} from "antd";
import {Dayjs} from "dayjs";

import {computeTimelineConfig} from "../../../../SearchResults/SearchResultsTimeline/utils";
import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../../../TimeRangeInput/utils";
import {
    buildPrestoGuidedQueries,
    handlePrestoGuidedQuerySubmit,
} from "../../Guided/presto-guided-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to run the guided SQL query.
 *
 * @return
 */
const GuidedRunButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const updateTimeRange = useSearchStore((state) => state.updateTimeRange);
    const updateTimelineConfig = useSearchStore((state) => state.updateTimelineConfig);
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);
    const timeRange = useSearchStore((state) => state.timeRange);
    const {selectDatasets} = useSearchStore.getState();
    const from = 0 < selectDatasets.length ?
        selectDatasets[0] :
        null;

    const select = usePrestoSearchState((state) => state.select);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateCachedGuidedSearchQueryString =
        usePrestoSearchState((state) => state.updateCachedGuidedSearchQueryString);

    const [messageApi, contextHolder] = message.useMessage();

    const isQueryReady =
        "" !== select.trim() &&
        null !== from &&
        null !== timestampKey;

    const tooltipTitle = false === isQueryReady ?
        "Enter minimal SQL fields to query" :
        "";

    const handleClick = useCallback(async () => {
        let newTimeRange: [Dayjs, Dayjs];
        if (timeRangeOption !== TIME_RANGE_OPTION.CUSTOM) {
            try {
                newTimeRange = await TIME_RANGE_OPTION_DAYJS_MAP[timeRangeOption]();
                updateTimeRange(newTimeRange);
            } catch {
                messageApi.warning(
                    "Cannot fetch the time range.",
                );

                return;
            }
        } else {
            newTimeRange = timeRange;
        }

        const {searchQueryString, timelineQueryString} = buildPrestoGuidedQueries(newTimeRange);
        handlePrestoGuidedQuerySubmit(searchQueryString, timelineQueryString);
        updateCachedGuidedSearchQueryString(searchQueryString);
        const newTimelineConfig = computeTimelineConfig(newTimeRange);
        updateTimelineConfig(newTimelineConfig);
    }, [
        messageApi,
        timeRange,
        timeRangeOption,
        updateTimeRange,
        updateTimelineConfig,
        updateCachedGuidedSearchQueryString,
    ]);

    return (
        <Tooltip title={tooltipTitle}>
            {contextHolder}
            <Button
                className={styles["runButton"] || ""}
                color={"green"}
                htmlType={"submit"}
                icon={<CaretRightOutlined/>}
                size={"middle"}
                variant={"solid"}
                disabled={!isQueryReady ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={() => {
                    handleClick().catch((err: unknown) => {
                        throw err;
                    });
                }}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default GuidedRunButton;

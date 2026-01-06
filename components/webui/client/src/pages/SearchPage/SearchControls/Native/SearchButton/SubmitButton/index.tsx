import {useCallback} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {
    Button,
    message,
    Tooltip,
} from "antd";
import {Dayjs} from "dayjs";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../../config";
import {computeTimelineConfig} from "../../../../SearchResults/SearchResultsTimeline/utils";
import useSearchStore from "../../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
} from "../../../TimeRangeInput/utils";
import {handleQuerySubmit} from "../../search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SubmitButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);
    const timeRange = useSearchStore((state) => state.timeRange);
    const updateTimeRange = useSearchStore((state) => state.updateTimeRange);
    const queryIsCaseSensitive = useSearchStore(
        (state) => state.queryIsCaseSensitive,
    );
    const queryString = useSearchStore((state) => state.queryString);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const updateCachedDataset = useSearchStore(
        (state) => state.updateCachedDataset,
    );
    const [messageApi, contextHolder] = message.useMessage();

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(async () => {
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

        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(newTimeRange);
        const {updateTimelineConfig} = useSearchStore.getState();
        updateTimelineConfig(newTimelineConfig);

        if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
            if (null !== selectDataset) {
                updateCachedDataset(selectDataset);
            } else {
                console.error(
                    "Cannot submit a clp-s query without a dataset selection.",
                );

                return;
            }
        }

        handleQuerySubmit({
            dataset: selectDataset,
            ignoreCase: false === queryIsCaseSensitive,
            queryString: queryString,
            timeRangeBucketSizeMillis: newTimelineConfig.bucketDuration.asMilliseconds(),
            ...(
                timeRangeOption === TIME_RANGE_OPTION.ALL_TIME ?
                    {
                        timestampBegin: null,
                        timestampEnd: null,
                    } :
                    {
                        timestampBegin: newTimeRange[0].valueOf(),
                        timestampEnd: newTimeRange[1].valueOf(),
                    }
            ),
        });
    }, [
        queryString,
        queryIsCaseSensitive,
        timeRange,
        timeRangeOption,
        updateTimeRange,
        messageApi,
        selectDataset,
        updateCachedDataset,
    ]);

    const isQueryStringEmpty = "" === queryString;

    // Submit button must be disabled if there are no datasets since clp-s requires dataset option
    // for queries.
    const isNoDatasetsAndClpS = null === selectDataset &&
        CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;

    let tooltipTitle = "";
    if (isNoDatasetsAndClpS) {
        tooltipTitle = "Some data must be ingested to enable search";
    } else if (isQueryStringEmpty) {
        tooltipTitle = "Enter query to search";
    }

    return (
        <Tooltip title={tooltipTitle}>
            {contextHolder}
            <Button
                className={styles["gradientButton"] || ""}
                htmlType={"submit"}
                icon={<SearchOutlined/>}
                size={"middle"}
                type={"primary"}
                disabled={isQueryStringEmpty ||
                    isNoDatasetsAndClpS ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING}
                onClick={() => {
                    handleSubmitButtonClick().catch((err: unknown) => {
                        throw err;
                    });
                }}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;

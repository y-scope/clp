import {useCallback} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {Nullable} from "@webui/common/utility-types";
import {
    Button,
    Tooltip,
} from "antd";
import dayjs, {Dayjs} from "dayjs";

import {querySql} from "../../../../../api/sql";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../../../config";
import {computeTimelineConfig} from "../../../SearchResults/SearchResultsTimeline/utils";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {handleQuerySubmit} from "../../search-requests";
import {TIME_RANGE_OPTION} from "../../TimeRangeInput/utils";
import styles from "./index.module.css";
import {
    buildClpsTimeRangeSql,
    buildClpTimeRangeSql,
} from "./timeRangeSql";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SubmitButton = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const timeRangeOption = useSearchStore((state) => state.timeRangeOption);
    const timeRange = useSearchStore((state) => state.timeRange);
    const queryIsCaseSensitive = useSearchStore(
        (state) => state.queryIsCaseSensitive,
    );
    const queryString = useSearchStore((state) => state.queryString);
    const selectDataset = useSearchStore((state) => state.selectDataset);
    const updateCachedDataset = useSearchStore(
        (state) => state.updateCachedDataset,
    );

    const fetchAllTimeTimeRange = useCallback(async (): Promise<
        [Dayjs, Dayjs]
    > => {
        let sql: string;
        if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
            sql = buildClpTimeRangeSql();
        } else {
            sql = buildClpsTimeRangeSql(selectDataset ?? "default");
        }
        const resp = await querySql<
            {
                begin_timestamp: Nullable<number>;
                end_timestamp: Nullable<number>;
            }[]
        >(sql);
        const [timestamps] = resp.data;
        if ("undefined" === typeof timestamps ||
            null === timestamps.begin_timestamp ||
            null === timestamps.end_timestamp
        ) {
            throw new Error();
        }

        return [
            dayjs.utc(timestamps.begin_timestamp),
            dayjs.utc(timestamps.end_timestamp),
        ];
    }, [selectDataset]);

    /**
     * Submits search query.
     */
    const handleSubmitButtonClick = useCallback(async () => {
        let updatedTimeRange: [Dayjs, Dayjs];
        if (timeRangeOption === TIME_RANGE_OPTION.ALL_TIME) {
            try {
                updatedTimeRange = await fetchAllTimeTimeRange();
            } catch (err: unknown) {
                console.error("Cannot fetch all time time range.", err);
                updatedTimeRange = timeRange;
            }
        } else {
            updatedTimeRange = timeRange;
        }

        // Update timeline to match range picker selection.
        const newTimelineConfig = computeTimelineConfig(updatedTimeRange);
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
            timestampBegin: timeRange[0].valueOf(),
            timestampEnd: timeRange[1].valueOf(),
        });
    }, [
        queryString,
        queryIsCaseSensitive,
        timeRange,
        timeRangeOption,
        fetchAllTimeTimeRange,
        selectDataset,
        updateCachedDataset,
    ]);

    const isQueryStringEmpty = "" === queryString;

    // Submit button must be disabled if there are no datasets since clp-s requires dataset option
    // for queries.
    const isNoDatasetsAndClpS =
        null === selectDataset &&
        CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;

    let tooltipTitle = "";
    if (isNoDatasetsAndClpS) {
        tooltipTitle = "Some data must be ingested to enable search";
    } else if (isQueryStringEmpty) {
        tooltipTitle = "Enter query to search";
    }

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["gradientButton"] || ""}
                htmlType={"submit"}
                icon={<SearchOutlined/>}
                size={"middle"}
                type={"primary"}
                disabled={
                    isQueryStringEmpty ||
                    isNoDatasetsAndClpS ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING
                }
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

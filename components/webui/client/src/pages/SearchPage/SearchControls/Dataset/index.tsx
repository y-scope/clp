import {useEffect} from "react";

import {useQuery} from "@tanstack/react-query";
import {
    message,
    Select,
} from "antd";

import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import InputLabel from "../../../../components/InputLabel";
import { theme } from "antd";
import styles from "./index.module.css";
import {fetchDatasetNames} from "./sql";


/**
 * Renders a dataset selector component that allows users to select from available datasets.
 *
 * @return
 */
const Dataset = () => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const updateDataset = useSearchStore((state) => state.updateSelectDataset);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const { token } = theme.useToken();

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
    });

    // Update the selected dataset to the first dataset in the response. The dataset is only
    // updated if it isn't already set (i.e., on initial response).
    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && null === dataset) {
                updateDataset(data[0]);
            }
        }
    }, [isSuccess,
        data,
        dataset,
        updateDataset]);

    // Display error message if the query fails since user querying is disabled if no datasets.
    useEffect(() => {
        if (error) {
            messageApi.error({
                key: "fetchError",
                content: "Error fetching datasets.",
            });
        }
    }, [error,
        messageApi]);

    // Display warning message if response empty since user querying is disabled if no
    // datasets.
    useEffect(() => {
        if (isSuccess && 0 === data.length) {
            messageApi.warning({
                key: "noData",
                content: "No data has been ingested. Please ingest data to search.",
            });

            // If all datasets were deleted, the dataset state must be reset to null to disable
            // submit button since dataset option required to query clp-s.
            updateDataset(null);
        }
    }, [data,
        isSuccess,
        messageApi,
        updateDataset]);

    const handleDatasetChange = (value: string) => {
        updateDataset(value);
    };

    return (
        <div className={styles["datasetContainer"]}>
            {contextHolder}
            <InputLabel fontSize={token.fontSizeLG}>Dataset</InputLabel>
            <Select
                className={styles["select"] || ""}
                loading={isPending}
                options={(data || []).map((option) => ({label: option, value: option}))}
                placeholder={"None"}
                showSearch={true}
                size={"large"}
                value={dataset}
                disabled={
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleDatasetChange}/>
        </div>
    );
};

export default Dataset;

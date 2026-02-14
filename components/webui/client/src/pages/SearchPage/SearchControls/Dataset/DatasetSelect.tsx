import {useEffect} from "react";

import {useQuery} from "@tanstack/react-query";
import {
    message,
    Select,
    SelectProps,
} from "antd";

import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";

import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {fetchDatasetNames} from "./sql";


/**
 * Renders a dataset selector component that allows users to select from available datasets.
 *
 * @param selectProps
 * @return
 */
const DatasetSelect = (selectProps: SelectProps) => {
    const datasets = useSearchStore((state) => state.selectDatasets);
    const updateDatasets = useSearchStore((state) => state.updateSelectDatasets);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
    });

    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && 0 === datasets.length) {
                const fallback = data.includes(CLP_DEFAULT_DATASET_NAME) ?
                    CLP_DEFAULT_DATASET_NAME :
                    data[0];
                updateDatasets([fallback]);
            }
        }
    }, [isSuccess,
        data,
        datasets,
        updateDatasets]);

    useEffect(() => {
        if (error) {
            messageApi.error({
                key: "fetchError",
                content: "Error fetching datasets.",
            });
        }
    }, [error,
        messageApi]);

    useEffect(() => {
        if (isSuccess && 0 === data.length) {
            messageApi.warning({
                key: "noData",
                content: "No data has been ingested. Please ingest data to search.",
            });
            updateDatasets([]);
        }
    }, [data,
        isSuccess,
        messageApi,
        updateDatasets]);

    const handleDatasetChange = (value: string[]) => {
        if (0 === value.length) {
            const fallback = (data || []).includes(CLP_DEFAULT_DATASET_NAME) ?
                CLP_DEFAULT_DATASET_NAME :
                data?.[0];

            if ("undefined" !== typeof fallback) {
                updateDatasets([fallback]);

                return;
            }
        }
        updateDatasets(value);
    };

    return (
        <>
            {contextHolder}
            <Select
                loading={isPending}
                mode={"multiple"}
                options={(data || []).map((option) => ({label: option, value: option}))}
                popupMatchSelectWidth={false}
                size={"middle"}
                value={datasets}
                disabled={
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleDatasetChange}
                {...selectProps}/>
        </>
    );
};

export default DatasetSelect;

import {useEffect} from "react";
import {useQuery} from "@tanstack/react-query";
import {message, Select, SelectProps} from "antd";
import useSearchStore from "../../SearchState/index";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {fetchDatasetNames} from "./sql";

const DatasetSelect = (selectProps: SelectProps) => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const updateDataset = useSearchStore((state) => state.updateSelectDataset);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
    });

    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && null === dataset) {
                updateDataset(data[0]);
            }
        }
    }, [isSuccess, data, dataset, updateDataset]);

    useEffect(() => {
        if (error) {
            messageApi.error({
                key: "fetchError",
                content: "Error fetching datasets.",
            });
        }
    }, [error, messageApi]);

    useEffect(() => {
        if (isSuccess && 0 === data.length) {
            messageApi.warning({
                key: "noData",
                content: "No data has been ingested. Please ingest data to search.",
            });
            updateDataset(null);
        }
    }, [data, isSuccess, messageApi, updateDataset]);

    const handleDatasetChange = (value: string) => {
        updateDataset(value);
    };

    return (
        <>
            {contextHolder}
            <Select
                loading={isPending}
                options={(data || []).map((option) => ({label: option, value: option}))}
                value={dataset}
                size="middle"
                disabled={
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleDatasetChange}
                {...selectProps}
            />
        </>
    );
};

export default DatasetSelect;

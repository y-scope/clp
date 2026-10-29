import {
    useCallback,
    useEffect,
} from "react";

import {useQuery} from "@tanstack/react-query";
import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    message,
    Select,
    SelectProps,
} from "antd";

import {SETTINGS_MAX_DATASETS_PER_QUERY} from "../../../../config";
import useSearchStore from "../../SearchState";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import {fetchDatasetNames} from "./sql";


interface DatasetSelectProps {
    isMultiSelect: boolean;
    className?: string;
    maxTagCount?: SelectProps["maxTagCount"];
}

/**
 * Renders a dataset selector component that allows users to select from available datasets.
 *
 * @param props
 * @param props.isMultiSelect
 * @param props.maxTagCount
 * @param props.selectProps
 * @return
 */
const DatasetSelect = ({isMultiSelect, maxTagCount, ...selectProps}: DatasetSelectProps) => {
    const datasets = useSearchStore((state) => state.selectedDatasets);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const updateDatasets = useSearchStore((state) => state.updateSelectedDatasets);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
    });

    /**
     * Returns a single-element fallback dataset list, preferring the default dataset name.
     * Returns an empty array if no datasets are available.
     *
     * @return
     */
    const getFallbackDatasets = useCallback((): string[] => {
        const available = data || [];
        if (0 === available.length) {
            return [];
        }

        return available.includes(CLP_DEFAULT_DATASET_NAME) ?
            [CLP_DEFAULT_DATASET_NAME] :
            [available[0] as string];
    }, [data]);

    // Set the initial selection when data first loads.
    useEffect(() => {
        if (isSuccess && 0 < data.length && 0 === datasets.length) {
            updateDatasets(getFallbackDatasets());
        }
    }, [isSuccess,
        data,
        datasets,
        getFallbackDatasets,
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

    const handleMultiDatasetChange = (value: string[]) => {
        if (null !== SETTINGS_MAX_DATASETS_PER_QUERY &&
            value.length > SETTINGS_MAX_DATASETS_PER_QUERY
        ) {
            messageApi.warning({
                key: "maxDatasetsExceeded",
                content: `Maximum of ${SETTINGS_MAX_DATASETS_PER_QUERY} datasets can be` +
                    " selected per query.",
            });

            return;
        }
        updateDatasets(0 === value.length ?
            getFallbackDatasets() :
            value);
    };

    const handleSingleDatasetChange = (value: string) => {
        updateDatasets([value]);
    };

    const isDisabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    return (
        <>
            {contextHolder}
            {isMultiSelect ?
                <Select
                    disabled={isDisabled}
                    loading={isPending}
                    mode={"multiple"}
                    options={(data || []).map((option) => ({label: option, value: option}))}
                    popupMatchSelectWidth={false}
                    size={"middle"}
                    value={datasets}
                    onChange={handleMultiDatasetChange}
                    {...selectProps}
                    {...("undefined" !== typeof maxTagCount ?
                        {maxTagCount: maxTagCount} :
                        {})}/> :
                <Select
                    disabled={isDisabled}
                    loading={isPending}
                    options={(data || []).map((option) => ({label: option, value: option}))}
                    popupMatchSelectWidth={false}
                    size={"middle"}
                    value={datasets[0] ?? null}
                    onChange={handleSingleDatasetChange}
                    {...selectProps}/>}
        </>
    );
};

export default DatasetSelect;

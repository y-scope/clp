import {useEffect} from "react";

import {useQuery} from "@tanstack/react-query";
import {
    message,
    Select,
    SelectProps,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto/index";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {fetchTimestampColumns} from "./sql";


/**
 * Renders a timestamp key selector component
 *
 * @param selectProps
 * @return
 */
const TimestampKeySelect = (selectProps: SelectProps) => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: [
            "timestampColumns",
            dataset
        ],
        queryFn: () => fetchTimestampColumns(dataset!),
        enabled: Boolean(dataset),
    });

    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && null === timestampKey) {
                updateTimestampKey(data[0]);
            }
        }
    }, [
        isSuccess,
        data,
        timestampKey,
        updateTimestampKey
    ]);

    useEffect(() => {
        if (error) {
            messageApi.error({
                key: "fetchTimestampError",
                content: "Error fetching timestamp columns.",
            });
        }
    }, [
        error,
        messageApi
    ]);

    useEffect(() => {
        if (isSuccess && 0 === data.length) {
            messageApi.warning({
                key: "noTimestamps",
                content: "No timestamp columns found for selected dataset. Guided UI requires a timestamp column.",
            });
            updateTimestampKey(null);
        }
    }, [
        data,
        isSuccess,
        messageApi,
        updateTimestampKey
    ]);

    // Reset timestamp key when dataset changes
    useEffect(() => {
        updateTimestampKey(null);
    }, [
        dataset,
        updateTimestampKey
    ]);

    const handleTimestampKeyChange = (value: string) => {
        updateTimestampKey(value);
    };

    return (
        <>
            {contextHolder}
            <Select
                loading={isPending}
                options={(data || []).map((option) => ({label: option, value: option}))}
                value={timestampKey}
                disabled={
                    !dataset ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleTimestampKeyChange}
                {...selectProps}/>
        </>
    );
};

export default TimestampKeySelect;

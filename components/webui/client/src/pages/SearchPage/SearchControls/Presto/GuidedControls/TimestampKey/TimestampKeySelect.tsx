import {useEffect} from "react";

import {useQuery} from "@tanstack/react-query";
import {
    message,
    Select,
    SelectProps,
} from "antd";

import useSearchStore from "../../../../SearchState";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {fetchTimestampColumns} from "./sql";


/**
 * Renders a timestamp key selector component
 *
 * @param selectProps
 * @return
 */
const TimestampKeySelect = (selectProps: SelectProps<string>) => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    const [messageApi, contextHolder] = message.useMessage();

    const {data: timestampKeys, isPending, isSuccess, isError} = useQuery({
        queryKey: [
            "timestampColumns",
            dataset,
        ],
        queryFn: () => (dataset ?
            fetchTimestampColumns(dataset) :
            []),
        enabled: null !== dataset,
    });

    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof timestampKeys[0] && null === timestampKey) {
                updateTimestampKey(timestampKeys[0]);
            }
        }
    }, [
        isSuccess,
        timestampKeys,
        timestampKey,
        updateTimestampKey,
    ]);

    useEffect(() => {
        if (isError) {
            messageApi.error({
                key: "fetchTimestampError",
                content: "Error fetching timestamp columns.",
            });
        }
    }, [
        isError,
        messageApi,
    ]);

    useEffect(() => {
        if (isSuccess && 0 === timestampKeys.length) {
            messageApi.warning({
                key: "noTimestamps",
                content: "No timestamp columns found for selected dataset. " +
                         "Guided UI requires a timestamp column.",
            });
            updateTimestampKey(null);
        }
    }, [
        timestampKeys,
        isSuccess,
        messageApi,
        updateTimestampKey,
    ]);

    // Reset timestamp key when dataset changes
    useEffect(() => {
        updateTimestampKey(null);
    }, [
        dataset,
        updateTimestampKey,
    ]);

    const handleTimestampKeyChange = (value: string) => {
        updateTimestampKey(value);
    };

    return (
        <>
            {contextHolder}
            <Select<string>
                loading={isPending}
                options={(timestampKeys || []).map((option) => ({label: option, value: option}))}
                value={timestampKey}
                disabled={
                    null === dataset ||
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleTimestampKeyChange}
                {...selectProps}/>
        </>
    );
};

export default TimestampKeySelect;

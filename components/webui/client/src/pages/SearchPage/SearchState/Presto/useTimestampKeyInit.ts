import {useEffect} from "react";

import {message} from "antd";
import {useQuery} from "@tanstack/react-query";

import {fetchTimestampColumns} from "../../SearchControls/TimeRangeInput/Presto/TimeRangeFooter/TimestampKeySelect/sql";
import useSearchStore from "..";
import usePrestoSearchState from ".";


/**
 * Hook to initialize and manage timestamp key selection.
 * Uses React Query to fetch timestamp columns and sets a default if none is selected.
 * This should be called once at the GuidedControls level.
 */
const useTimestampKeyInit = () => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);

    const [messageApi, contextHolder] = message.useMessage();

    const {data: timestampKeys, isSuccess, isError} = useQuery({
        queryKey: [
            "timestampColumns",
            dataset,
        ],
        queryFn: () => (dataset ?
            fetchTimestampColumns(dataset) :
            []),
        enabled: null !== dataset,
    });

    // Set default timestamp key when data is loaded
    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof timestampKeys?.[0] && null === timestampKey) {
                updateTimestampKey(timestampKeys[0] ?? null);
            }
        }
    }, [
        isSuccess,
        timestampKeys,
        timestampKey,
        updateTimestampKey,
    ]);

    // Show error message if fetch fails
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

    // Show warning if no timestamp columns found
    useEffect(() => {
        if (isSuccess && 0 === (timestampKeys?.length ?? 0)) {
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

    return {contextHolder};
};

export default useTimestampKeyInit;

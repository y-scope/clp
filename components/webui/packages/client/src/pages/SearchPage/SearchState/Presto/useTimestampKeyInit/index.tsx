import {useEffect} from "react";

import {useQuery} from "@tanstack/react-query";
import {message} from "antd";

import useSearchStore from "../../";
import usePrestoSearchState from "..";
import {fetchTimestampColumns} from "./sql";


/**
 * Hook to initialize timestamp key. Fetches timestamp columns and sets a default if none is
 * selected. Shows messages for errors or warnings with data fetching.
 *
 * @return
 */
const useTimestampKeyInit = () => {
    const selectedDatasets = useSearchStore((state) => state.selectedDatasets);
    const dataset = 0 < selectedDatasets.length ?
        selectedDatasets[0] :
        null;
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

    // Reset timestamp key when dataset changes.
    useEffect(() => {
        updateTimestampKey(null);
    }, [
        dataset,
        updateTimestampKey,
    ]);

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

    return {contextHolder};
};

export default useTimestampKeyInit;

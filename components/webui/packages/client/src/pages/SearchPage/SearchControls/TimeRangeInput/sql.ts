import dayjs, {Dayjs} from "dayjs";

import {apiClient} from "../../../../api/search";
import {buildApiErrorMessage} from "../../../../api/utils";
import {DEFAULT_TIME_RANGE} from "./utils";


/**
 * Fetches the earliest and latest log entry timestamps ("all time" range)
 * from the configured storage engine (CLP or CLPS).
 *
 * @param selectedDatasets
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchAllTimeRange = async (selectedDatasets: string[]): Promise<[Dayjs, Dayjs]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/time_range", {
        params: {query: {dataset: selectedDatasets.join(",")}},
    });

    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch time range", error, response));
    }

    if (null === data.begin_timestamp || null === data.end_timestamp) {
        return DEFAULT_TIME_RANGE;
    }

    return [
        dayjs.utc(data.begin_timestamp),
        dayjs.utc(data.end_timestamp),
    ];
};

export {fetchAllTimeRange};

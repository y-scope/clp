import {apiClient} from "../../../../../api/search";
import {buildApiErrorMessage} from "../../../../../api/utils";


/**
 * Fetches timestamp column names for a specific dataset.
 *
 * @param datasetName
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchTimestampColumns = async (datasetName: string): Promise<string[]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET(
        "/metadata/column_metadata/{dataset_name}/timestamp",
        {params: {path: {dataset_name: datasetName}}},
    );

    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch timestamp columns", error, response));
    }

    return data;
};

export {fetchTimestampColumns};

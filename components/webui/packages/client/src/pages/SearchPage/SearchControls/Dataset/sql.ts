import {apiClient} from "../../../../api/search";
import {buildApiErrorMessage} from "../../../../api/utils";


/**
 * Fetches all dataset names from the datasets table.
 *
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchDatasetNames = async (): Promise<string[]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/datasets", {});
    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch dataset names", error, response));
    }

    return data;
};

export {fetchDatasetNames};

import {apiClient} from "../../../../api/search";


/**
 * Fetches all dataset names from the datasets table.
 *
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchDatasetNames = async (): Promise<string[]> => {
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.GET("/metadata/datasets", {});
    if ("undefined" === typeof data) {
        throw new Error(`Failed to fetch dataset names: HTTP ${response.status}`);
    }

    return data;
};

export {fetchDatasetNames};

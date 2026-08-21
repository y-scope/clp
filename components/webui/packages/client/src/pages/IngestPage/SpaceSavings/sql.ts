import {apiClient} from "../../../api/search";
import {buildApiErrorMessage} from "../../../api/utils";


/**
 * Result from sql space savings query.
 */
interface SpaceSavingsItem {
    total_uncompressed_size: number;
    total_compressed_size: number;
}

/**
 * Default values for space savings when no data is available.
 */
const SPACE_SAVINGS_DEFAULT: SpaceSavingsItem = {
    total_compressed_size: 0,
    total_uncompressed_size: 0,
};


/**
 * Fetches space savings statistics when using CLP storage engine.
 *
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchClpSpaceSavings = async (): Promise<SpaceSavingsItem> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/space_savings", {});
    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch space savings", error, response));
    }

    return {
        total_compressed_size: data.total_compressed_size,
        total_uncompressed_size: data.total_uncompressed_size,
    };
};

/**
 * Fetches space savings statistics when using CLP-S storage engine.
 *
 * @param datasetNames
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchClpsSpaceSavings = async (
    datasetNames: string[]
): Promise<SpaceSavingsItem> => {
    if (0 === datasetNames.length) {
        return SPACE_SAVINGS_DEFAULT;
    }

    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/space_savings", {
        params: {query: {dataset: datasetNames.join(",")}},
    });

    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch space savings", error, response));
    }

    return {
        total_compressed_size: data.total_compressed_size,
        total_uncompressed_size: data.total_uncompressed_size,
    };
};

export type {SpaceSavingsItem};
export {
    fetchClpSpaceSavings,
    fetchClpsSpaceSavings,
    SPACE_SAVINGS_DEFAULT,
};

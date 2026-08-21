import {Nullable} from "@webui/common/utility-types";

import {apiClient} from "../../../api/search";
import {buildApiErrorMessage} from "../../../api/utils";


/**
 * Result from SQL details query.
 */
interface DetailsItem {
    begin_timestamp: Nullable<number>;
    end_timestamp: Nullable<number>;
    num_files: Nullable<number>;
    num_messages: Nullable<number>;
}

/**
 * Default values for details when no data is available.
 */
const DETAILS_DEFAULT: DetailsItem = {
    begin_timestamp: null,
    end_timestamp: null,
    num_files: 0,
    num_messages: 0,
};


/**
 * Fetches details statistics when using CLP storage engine.
 *
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchClpDetails = async (): Promise<DetailsItem> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/ingestion_details", {});
    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch details", error, response));
    }

    return {
        begin_timestamp: data.begin_timestamp ?? null,
        end_timestamp: data.end_timestamp ?? null,
        num_files: data.num_files ?? null,
        num_messages: data.num_messages ?? null,
    };
};

/**
 * Fetches details statistics when using CLP-S storage engine.
 *
 * @param datasetNames
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchClpsDetails = async (
    datasetNames: string[]
): Promise<DetailsItem> => {
    if (0 === datasetNames.length) {
        return DETAILS_DEFAULT;
    }

    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/metadata/ingestion_details", {
        params: {query: {dataset: datasetNames.join(",")}},
    });

    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to fetch details", error, response));
    }

    return {
        begin_timestamp: data.begin_timestamp ?? null,
        end_timestamp: data.end_timestamp ?? null,
        num_files: data.num_files ?? null,
        num_messages: data.num_messages ?? null,
    };
};

export type {DetailsItem};
export {
    DETAILS_DEFAULT,
    fetchClpDetails,
    fetchClpsDetails,
};

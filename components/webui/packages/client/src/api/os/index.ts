import type {components} from "@webui/api-client";

import {apiClient} from "../search";
import {createApiError} from "../utils";


type DirEntry = components["schemas"]["DirEntry"];

/**
 * Lists files and directories at the specified path.
 *
 * @param path
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const listFiles = async (path: string): Promise<DirEntry[]> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/os/ls", {params: {query: {path}}});
    if ("undefined" === typeof data) {
        throw createApiError("Failed to list files", response, error);
    }

    return data;
};

export type {DirEntry};
export {listFiles};

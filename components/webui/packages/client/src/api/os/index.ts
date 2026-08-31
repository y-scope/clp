import {FileListing} from "@webui/common/schemas/os";

import {apiClient} from "../search";


/**
 * Lists files and directories at the specified path.
 *
 * @param path
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const listFiles = async (path: string): Promise<FileListing> => {
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.GET("/os/ls", {params: {query: {path}}});
    if ("undefined" === typeof data) {
        throw new Error(`Failed to list files: HTTP ${response.status}`);
    }

    return data;
};

export {listFiles};

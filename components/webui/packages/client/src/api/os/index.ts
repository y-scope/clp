import type {components} from "@webui/api-client";

import {apiClient} from "../search";
import {buildApiErrorMessage} from "../utils";


type FileEntry = components["schemas"]["DirEntry"];

type FileListing = FileEntry[];


/**
 * Lists files and directories at the specified path.
 *
 * @param path
 * @return
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const listFiles = async (path: string): Promise<FileListing> => {
    // eslint-disable-next-line new-cap
    const {data, error, response} = await apiClient.GET("/os/ls", {params: {query: {path}}});
    if ("undefined" === typeof data) {
        throw new Error(buildApiErrorMessage("Failed to list files", error, response));
    }

    return data;
};

export {listFiles};
export type {
    FileEntry,
    FileListing,
};

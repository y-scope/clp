import {
    ComponentsListing,
    LogContentResponse,
    LogFilesListing,
} from "@webui/common/schemas/operational-logs";
import axios from "axios";


/**
 * Lists available components with operational logs.
 *
 * @return Promise resolving to list of components
 */
const listComponents = async (): Promise<ComponentsListing> => {
    const {data} = await axios.get<ComponentsListing>("/api/operational-logs/components");

    return data;
};

/**
 * Lists log files, optionally filtered by component.
 *
 * @param component Optional component name to filter by
 * @return Promise resolving to list of log files
 */
const listLogFiles = async (component?: string): Promise<LogFilesListing> => {
    const {data} = await axios.get<LogFilesListing>("/api/operational-logs/files", {
        params: {component},
    });

    return data;
};

/**
 * Reads log content from a file.
 *
 * @param path The path to the log file
 * @param offset Line offset to start from
 * @param limit Maximum number of lines to return
 * @return Promise resolving to log content
 */
const readLogContent = async (
    path: string,
    offset = 0,
    limit = 100
): Promise<LogContentResponse> => {
    const {data} = await axios.get<LogContentResponse>("/api/operational-logs/content", {
        params: {path, offset, limit},
    });

    return data;
};

export {
    listComponents,
    listLogFiles,
    readLogContent,
};

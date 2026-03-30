import {FileEntry} from "@webui/common/schemas/os";

import {settings} from "../../../../settings";
import {
    ROOT_PATH,
    TreeNode,
} from "./typings";


/**
 * Percentage of viewport height to use for the dropdown list.
 */
const LIST_HEIGHT_VIEWPORT_PERCENT = 0.5;

const MIN_LIST_HEIGHT_PX = 200;
const MAX_LIST_HEIGHT_PX = 600;


/**
 * Calculates list height based on window height.
 *
 * @return The calculated height clamped between min and max values.
 */
const getListHeight = (): number => {
    const calculatedHeight = Math.round(window.innerHeight * LIST_HEIGHT_VIEWPORT_PERCENT);

    return Math.max(
        MIN_LIST_HEIGHT_PX,
        Math.min(MAX_LIST_HEIGHT_PX, calculatedHeight)
    );
};

/**
 * Joins path segments into a normalized POSIX path. Empty segments are ignored and consecutive
 * slashes are collapsed.
 *
 * @param parts Path segments to join
 * @return The normalized path
 */
const joinPath = (...parts: string[]): string => parts
    .filter(Boolean)
    .join("/")
    .replace(/\/{2,}/g, "/");

/**
 * Removes the LogsInputRootDir prefix from a server path.
 *
 * @param serverPath The full server path (e.g., "/mnt/logs/mydir")
 * @return The user-facing path (e.g., "/mydir")
 * @throws Error if `LogsInputRootDir` is not configured.
 */
const removeServerPrefix = (serverPath: string): string => {
    const logsInputRootDir = settings.LogsInputRootDir;
    if (null === logsInputRootDir) {
        throw new Error("LogsInputRootDir is not configured.");
    }

    const stripped = serverPath.startsWith(logsInputRootDir) ?
        serverPath.slice(logsInputRootDir.length) :
        serverPath;

    return joinPath(ROOT_PATH, stripped);
};

/**
 * Adds the LogsInputRootDir prefix to a user-facing path.
 *
 * @param userPath The user-facing path (e.g., "/mydir")
 * @return The full server path (e.g., "/mnt/logs/mydir")
 * @throws Error if `LogsInputRootDir` is not configured.
 */
const addServerPrefix = (userPath: string): string => {
    const logsInputRootDir = settings.LogsInputRootDir;
    if (null === logsInputRootDir) {
        throw new Error("LogsInputRootDir is not configured.");
    }

    return joinPath(logsInputRootDir, userPath);
};

/**
 * Converts API file listing item to an Ant Design TreeSelect node.
 *
 * @param fileEntry File item from the `/os/ls` API response
 * @param parentPath User-facing parent path for tree hierarchy
 * @return TreeNode with normalized path as id/value
 */
const toTreeNode = (fileEntry: FileEntry, parentPath: string): TreeNode => {
    const fullPath = joinPath(removeServerPrefix(fileEntry.parentPath), fileEntry.name);

    return {
        id: fullPath,
        isLeaf: false === fileEntry.isExpandable,
        pId: parentPath,
        title: fileEntry.name,
        value: fullPath,
    };
};


export {
    addServerPrefix,
    getListHeight,
    ROOT_PATH,
    toTreeNode,
};

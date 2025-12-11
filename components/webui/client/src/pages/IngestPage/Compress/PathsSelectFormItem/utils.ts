import {FileEntry} from "@webui/common/schemas/os";
import {
    GetProp,
    TreeSelectProps,
} from "antd";

import {settings} from "../../../../settings";
import {ROOT_PATH} from "./typings";


/**
 * Tree node for Ant Design TreeSelect in simple mode (treeDataSimpleMode).
 */
interface TreeNode extends Omit<GetProp<TreeSelectProps, "treeData">[number], "label"> {
    id: string;
}


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


export type {TreeNode};
export {
    addServerPrefix,
    ROOT_PATH,
    toTreeNode,
};

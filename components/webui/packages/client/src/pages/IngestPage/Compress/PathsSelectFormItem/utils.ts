import {FileEntry} from "@webui/common/schemas/os";

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
 * Converts API file listing item to an Ant Design TreeSelect node.
 *
 * @param fileEntry File item from the `/os/ls` API response
 * @param parentPath Parent path for tree hierarchy
 * @return TreeNode with normalized path as id/value
 */
const toTreeNode = (fileEntry: FileEntry, parentPath: string): TreeNode => {
    const fullPath = joinPath(fileEntry.parent_path, fileEntry.name);

    return {
        id: fullPath,
        isLeaf: false === fileEntry.is_expandable,
        pId: parentPath,
        title: fileEntry.name,
        value: fullPath,
    };
};


export {
    getListHeight,
    ROOT_PATH,
    toTreeNode,
};

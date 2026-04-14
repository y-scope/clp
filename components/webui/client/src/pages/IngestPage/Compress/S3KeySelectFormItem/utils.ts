import {S3Entry} from "@webui/common/schemas/s3";

import {
    ROOT_ID,
    ROOT_PREFIX,
    S3TreeNode,
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
    const calculated = Math.round(window.innerHeight * LIST_HEIGHT_VIEWPORT_PERCENT);

    return Math.max(MIN_LIST_HEIGHT_PX, Math.min(MAX_LIST_HEIGHT_PX, calculated));
};

/**
 * Extracts the display name from an S3 key or prefix.
 *
 * @param key Full S3 key or prefix.
 * @param parentPrefix The parent prefix to strip.
 * @return Display name for the tree node.
 */
const getDisplayName = (key: string, parentPrefix: string): string => {
    const relative = key.startsWith(parentPrefix) ?
        key.slice(parentPrefix.length) :
        key;

    return relative || key;
};

/**
 * Converts S3 entries to tree nodes.
 *
 * @param entries S3 entries from the listing API.
 * @param parentPrefix Parent prefix for tree hierarchy.
 * @return Array of tree nodes.
 */
const toTreeNodes = (entries: S3Entry[], parentPrefix: string): S3TreeNode[] => {
    const parentId = ROOT_PREFIX === parentPrefix ?
        ROOT_ID :
        parentPrefix;

    return entries.map((entry) => ({
        id: entry.key,
        isLeaf: false === entry.isPrefix,
        isPrefix: entry.isPrefix,
        label: "/" + entry.key,
        pId: parentId,
        title: getDisplayName(entry.key, parentPrefix),
        value: entry.key,
    }));
};


export {
    getListHeight,
    toTreeNodes,
};

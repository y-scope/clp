import {
    GetProp,
    TreeSelectProps,
} from "antd";

import {settings} from "../../../settings";


type TreeNode = Omit<GetProp<TreeSelectProps, "treeData">[number], "label">;

interface FileItem {
    isExpandable: boolean;
    name: string;
    parentPath: string;
}


/**
 * Extracts the base directory path from a search string.
 *
 * @param value
 * @return the base directory path.
 */
const extractBasePath = (value: string): string => {
    if (value.endsWith("/")) {
        return "/" === value ?
            "/" :
            value.slice(0, -1);
    }

    const lastSlashIndex = value.lastIndexOf("/");
    if (-1 === lastSlashIndex || 0 === lastSlashIndex) {
        return "/";
    }

    return value.substring(0, lastSlashIndex);
};

/**
 * Joins multiple path segments into a single normalized POSIX file system path.
 *
 * @param parts
 * @return The normalized path.
 */
const joinPath = (...parts: string[]): string => parts
    .filter(Boolean)
    .join("/")
    .replace(/\/{2,}/g, "/");

/**
 * Normalizes a path for client display by removing the LogsInputRootDir prefix in a container
 * environment.
 *
 * @param fullPath
 * @return The normalized path relative to LogsInputRootDir.
 */
const removeLsPathPrefix = (fullPath: string): string => {
    return fullPath.replace(new RegExp(`^${settings.LogsInputRootDir}/*`), "/");
};

/**
 * Maps file system item to Antd TreeSelect flat tree node format.
 *
 * @param fileItem
 * @return the mapped Antd TreeSelect flat tree node.
 */
const mapFileToTreeNode = (fileItem: FileItem): TreeNode => {
    const {isExpandable, name, parentPath} = fileItem;
    const normalizedParentPath = 0 === parentPath.length ?
        "/" :
        removeLsPathPrefix(parentPath);
    const pathPrefix = normalizedParentPath.endsWith("/") ?
        normalizedParentPath :
        `${normalizedParentPath}/`;
    const fullPath = pathPrefix + name;

    return {
        id: fullPath,
        isLeaf: !isExpandable,
        pId: normalizedParentPath,
        title: name,
        value: fullPath,
    };
};


export type {
    FileItem,
    TreeNode,
};
export {
    extractBasePath,
    joinPath,
    mapFileToTreeNode,
    removeLsPathPrefix,
};

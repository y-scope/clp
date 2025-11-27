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
 * Normalizes a path for client display by removing the LsPathPrefixToRemove prefix in a container
 * environment.
 *
 * @param fullPath
 * @return The normalized path relative to LsRoot
 */
const removeLsPathPrefix = (fullPath: string): string => {
    return fullPath.replace(new RegExp(`^${settings.LsPathPrefixToRemove}/*`), "/");
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

export type {
    FileItem,
    TreeNode,
};
export {
    extractBasePath,
    mapFileToTreeNode,
    removeLsPathPrefix,
};

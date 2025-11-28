import {message} from "antd";
import {create} from "zustand";

import {listFiles} from "../../../api/os";
import {settings} from "../../../settings";
import {
    FileItem,
    mapFileToTreeNode,
    TreeNode,
} from "./utils";


/**
 * Default values for the file system tree state.
 */
const FILE_SYSTEM_TREE_STATE_DEFAULT = Object.freeze({
    expandedKeys: [],
    isLoading: false,
    loadedKeys: new Set<string>(),
    treeData: [{id: "/", value: "/", title: "/", isLeaf: false}] as TreeNode[],
});

interface FileSystemTreeValues {
    /**
     * Keys of expanded tree nodes.
     */
    expandedKeys: string[];

    /**
     * Whether the tree is currently loading data.
     */
    isLoading: boolean;

    /**
     * Keys of nodes that have been loaded.
     */
    loadedKeys: Set<string>;

    /**
     * Tree node data in flat format for TreeSelect.
     */
    treeData: TreeNode[];
}

interface FileSystemTreeActions {
    /**
     * Adds new tree nodes to the tree data.
     *
     * @param nodes
     */
    addTreeNodes: (nodes: TreeNode[]) => void;

    /**
     * Expands a node by adding its key to the expanded keys.
     *
     * @param key
     */
    expandNode: (key: string) => void;

    /**
     * Fetches and appends tree nodes for a given path.
     *
     * @param path
     * @return Whether the operation succeeded.
     */
    fetchAndAppendTreeNodes: (path: string) => Promise<boolean>;

    /**
     * Loads missing parent nodes for a given path.
     *
     * @param path
     * @return Whether the operation succeeded.
     */
    loadMissingParents: (path: string) => Promise<boolean>;

    /**
     * Sets the expanded keys for the tree.
     *
     * @param keys
     */
    setExpandedKeys: (keys: string[]) => void;

    /**
     * Sets the loading state.
     *
     * @param loading
     */
    setIsLoading: (loading: boolean) => void;
}

type FileSystemTreeState = FileSystemTreeValues & FileSystemTreeActions;


// eslint-disable-next-line max-lines-per-function
const useFileSystemTreeStore = create<FileSystemTreeState>((set, get) => ({
    ...FILE_SYSTEM_TREE_STATE_DEFAULT,

    addTreeNodes: (nodes) => {
        const {treeData} = get();
        const existingNodeIds = new Set(treeData.map((node) => node["id"] as string));
        const uniqueNewNodes = nodes.filter(
            (node) => !existingNodeIds.has(node["id"] as string)
        );

        set({treeData: [...treeData,
            ...uniqueNewNodes]});
    },

    expandNode: (key) => {
        const {expandedKeys} = get();
        if (!expandedKeys.includes(key)) {
            set({expandedKeys: [
                ...expandedKeys,
                key,
            ]});
        }
    },

    fetchAndAppendTreeNodes: async (path) => {
        const {loadedKeys} = get();
        if (loadedKeys.has(path)) {
            return false;
        }
        loadedKeys.add(path);

        try {
            const fullResolvedPath = settings.LogsInputRootDir + path;
            const fileItems = await listFiles(fullResolvedPath);
            const newNodes = (fileItems as FileItem[]).map(mapFileToTreeNode);

            get().addTreeNodes(newNodes);
            get().expandNode(path);

            return true;
        } catch (e) {
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while loading paths");

            return false;
        }
    },

    loadMissingParents: async (path) => {
        const {treeData, fetchAndAppendTreeNodes} = get();
        const pathSegments = path.split("/").filter((segment) => 0 < segment.length);

        let currentPath = "/";
        for (const segment of pathSegments) {
            const nextPath = "/" === currentPath ?
                `/${segment}` :
                `${currentPath}/${segment}`;

            if (false === treeData.some((node) => node["id"] === nextPath)) {
                const success = await fetchAndAppendTreeNodes(currentPath);
                if (false === success) {
                    return false;
                }
            }

            currentPath = nextPath;
        }

        return true;
    },

    setExpandedKeys: (keys) => {
        set({expandedKeys: keys});
    },

    setIsLoading: (loading) => {
        set({isLoading: loading});
    },
}));

export type {FileSystemTreeState};
export {FILE_SYSTEM_TREE_STATE_DEFAULT};
export default useFileSystemTreeStore;

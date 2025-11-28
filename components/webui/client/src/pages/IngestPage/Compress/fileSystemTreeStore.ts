import {message} from "antd";
import {create} from "zustand";

import {listFiles} from "../../../api/os";
import {settings} from "../../../settings";
import {
    FileItem,
    joinPath,
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
     * Keys of directory nodes whose children have been fetched from the server. Used to prevent
     * redundant API calls when expanding/collapsing nodes.
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
     * @return Whether the tree was updated.
     */
    fetchAndAppendTreeNodes: (path: string) => Promise<boolean>;

    /**
     * Handles tree expansion events by updating the expanded keys and fetching missing nodes.
     *
     * @param newKeys
     */
    handleTreeExpansion: (expandedPaths: string[]) => void;

    /**
     * Loads missing parent nodes for a given path.
     *
     * @param path
     * @return Whether the tree was updated.
     */
    loadMissingParents: (path: string) => Promise<boolean>;

    /**
     * Toggles the state of a node identified by the given key.
     *
     * @param {string} key
     */
    toggleNode: (key: string) => void;

    setExpandedKeys: (keys: string[]) => void;
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
        set({loadedKeys: loadedKeys});

        try {
            const fullResolvedPath = joinPath(settings.LogsInputRootDir, path);
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

    handleTreeExpansion: (expandedPaths) => {
        const {expandedKeys, fetchAndAppendTreeNodes, setExpandedKeys} = get();
        const newlyExpandedKeys = expandedPaths.filter((key) => !expandedKeys.includes(key));

        newlyExpandedKeys.forEach((key) => {
            fetchAndAppendTreeNodes(key).catch((e: unknown) => {
                console.error(e);
                if (e instanceof Error) {
                    message.error(`Failed to load directory: ${e.message}`);
                }
            });
        });

        setExpandedKeys(expandedPaths);
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

    toggleNode: (key) => {
        const {expandedKeys, fetchAndAppendTreeNodes, setExpandedKeys} = get();
        const isExpanded = expandedKeys.includes(key);

        if (isExpanded) {
            setExpandedKeys(expandedKeys.filter((k) => k !== key));
        } else {
            fetchAndAppendTreeNodes(key).catch((e: unknown) => {
                console.error(e);
                if (e instanceof Error) {
                    message.error(`Failed to load directory: ${e.message}`);
                }
            });
            setExpandedKeys([...expandedKeys,
                key]);
        }
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

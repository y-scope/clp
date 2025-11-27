import {
    useCallback,
    useEffect,
    useState,
} from "react";

import {
    message,
    TreeSelectProps,
} from "antd";

import {listFiles} from "../../../api/os";
import {settings} from "../../../settings";
import {
    extractBasePath,
    FileItem,
    mapFileToTreeNode,
    TreeNode,
} from "./utils";


interface TreeDataReturn {
    expandedKeys: string[];
    treeData: TreeNode[];
    fetchAndAppendTreeNodes: (path: string) => Promise<boolean>;
    loadMissingParents: (path: string) => Promise<boolean>;
    setExpandedKeys: (keys: string[] | ((prev: string[]) => string[])) => void;
}

interface TreeHandlersParams {
    expandedKeys: string[];
    fetchAndAppendTreeNodes: (path: string) => Promise<boolean>;
    loadMissingParents: (path: string) => Promise<boolean>;
    setExpandedKeys: (keys: string[] | ((prev: string[]) => string[])) => void;
    setIsLoading: (loading: boolean) => void;
}

interface TreeHandlersReturn {
    handleLoadData: NonNullable<TreeSelectProps["loadData"]>;
    handleSearch: NonNullable<TreeSelectProps["onSearch"]>;
    handleTreeExpand: NonNullable<TreeSelectProps["onTreeExpand"]>;
    toggleNodeExpansion: (key: string, expanded: boolean) => void;
}

interface FileSystemTree {
    isLoading: boolean;
    expandedKeys: string[];
    treeData: TreeNode[];
    handleLoadData: NonNullable<TreeSelectProps["loadData"]>;
    handleSearch: NonNullable<TreeSelectProps["onSearch"]>;
    handleTreeExpand: NonNullable<TreeSelectProps["onTreeExpand"]>;
    toggleNodeExpansion: (key: string, expanded: boolean) => void;
}

/**
 * Manages tree data loading and expansion.
 *
 * @return tree data and operations
 */
const useTreeData = (): TreeDataReturn => {
    const [treeData, setTreeData] = useState<TreeNode[]>([
        {id: "/", value: "/", title: "/", isLeaf: false},
    ]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);

    const fetchAndAppendTreeNodes = useCallback(async (path: string): Promise<boolean> => {
        try {
            const fullResolvedPath = settings.LsPathPrefixToRemove + path;
            const fileItems = await listFiles(fullResolvedPath);
            const newNodes = (fileItems as FileItem[]).map(mapFileToTreeNode);

            setTreeData((prev) => {
                const existingNodeIds = new Set(prev.map((node) => node["id"] as string));
                const uniqueNewNodes = newNodes.filter(
                    (node) => !existingNodeIds.has(node["id"] as string)
                );

                return [
                    ...prev,
                    ...uniqueNewNodes,
                ];
            });

            setExpandedKeys((prev) => Array.from(
                new Set([...prev,
                    path])
            ));

            return true;
        } catch (e) {
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while loading paths");

            return false;
        }
    }, []);

    const loadMissingParents = useCallback(async (path: string): Promise<boolean> => {
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
    }, [
        treeData,
        fetchAndAppendTreeNodes,
    ]);

    return {
        expandedKeys,
        fetchAndAppendTreeNodes,
        loadMissingParents,
        setExpandedKeys,
        treeData,
    };
};

/**
 * Custom hook for tree event handlers.
 *
 * @param params
 * @param params.expandedKeys
 * @param params.setExpandedKeys
 * @param params.fetchAndAppendTreeNodes
 * @param params.loadMissingParents
 * @param params.setIsLoading
 * @return event handlers
 */
const useTreeHandlers = (params: TreeHandlersParams): TreeHandlersReturn => {
    const {
        expandedKeys,
        setExpandedKeys,
        fetchAndAppendTreeNodes,
        loadMissingParents,
        setIsLoading,
    } = params;

    const handleLoadData = useCallback<NonNullable<TreeSelectProps["loadData"]>>(
        async (node) => {
            const path = node.value;
            if ("string" !== typeof path) {
                return;
            }
            setIsLoading(true);
            try {
                await fetchAndAppendTreeNodes(path);
            } finally {
                setIsLoading(false);
            }
        },
        [fetchAndAppendTreeNodes,
            setIsLoading]
    );

    const handleSearch = useCallback<NonNullable<TreeSelectProps["onSearch"]>>(
        (value) => {
            if (0 === value.trim().length) {
                return;
            }

            setIsLoading(true);
            (async () => {
                try {
                    const basePath = extractBasePath(value);
                    const parentsLoaded = await loadMissingParents(basePath);
                    if (parentsLoaded && false === expandedKeys.includes(basePath)) {
                        await fetchAndAppendTreeNodes(basePath);
                    }
                } finally {
                    setIsLoading(false);
                }
            })().catch((e: unknown) => {
                console.error("Failed to load missing parents", e);
            });
        },
        [
            fetchAndAppendTreeNodes,
            loadMissingParents,
            expandedKeys,
            setIsLoading,
        ]
    );

    const handleTreeExpand = useCallback<NonNullable<TreeSelectProps["onTreeExpand"]>>(
        (keys) => {
            setExpandedKeys(keys as string[]);
        },
        [setExpandedKeys]
    );

    const toggleNodeExpansion = useCallback((key: string, expanded: boolean) => {
        if (expanded) {
            setExpandedKeys((prev) => prev.filter((k) => k !== key));
        } else {
            setExpandedKeys((prev) => [...prev,
                key]);
        }
    }, [setExpandedKeys]);

    return {
        handleLoadData,
        handleSearch,
        handleTreeExpand,
        toggleNodeExpansion,
    };
};

/**
 * Provides a tree of file system items for inputting paths for compression job submission.
 *
 * @return
 */
const useFileSystemTree = (): FileSystemTree => {
    const [isLoading, setIsLoading] = useState(false);
    const {
        treeData,
        expandedKeys,
        setExpandedKeys,
        fetchAndAppendTreeNodes,
        loadMissingParents,
    } = useTreeData();

    const {
        handleLoadData,
        handleSearch,
        handleTreeExpand,
        toggleNodeExpansion,
    } = useTreeHandlers({
        expandedKeys,
        fetchAndAppendTreeNodes,
        loadMissingParents,
        setExpandedKeys,
        setIsLoading,
    });

    useEffect(() => {
        fetchAndAppendTreeNodes("/").catch((e: unknown) => {
            console.error("Failed to load root directory", e);
        });
    }, [fetchAndAppendTreeNodes]);

    return {
        expandedKeys,
        handleLoadData,
        handleSearch,
        handleTreeExpand,
        isLoading,
        toggleNodeExpansion,
        treeData,
    };
};

export default useFileSystemTree;

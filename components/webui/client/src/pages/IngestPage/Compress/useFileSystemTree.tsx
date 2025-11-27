import {
    useCallback,
    useState,
} from "react";

import {
    GetProp,
    message,
    TreeSelectProps,
} from "antd";

import {listFiles} from "../../../api/os";


type TreeNode = Omit<GetProp<TreeSelectProps, "treeData">[number], "label">;

interface FileItem {
    isExpandable: boolean;
    name: string;
    parentPath: string;
}

/**
 * Maps file system item to Antd TreeSelect flat tree node format.
 *
 * @param props
 * @param props.isExpandable
 * @param props.name
 * @param props.parentPath
 * @return the mapped Antd TreeSelect flat tree node.
 */
const mapFileToTreeNode = ({
    isExpandable,
    name,
    parentPath,
}: {
    isExpandable: boolean;
    name: string;
    parentPath: string;
}): TreeNode => {
    const normalizedParentPath = 0 === parentPath.length ?
        "/" :
        parentPath;
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

interface FileSystemTree {
    expandedKeys: string[];
    handleLoadData: NonNullable<TreeSelectProps["loadData"]>;
    handleSearch: NonNullable<TreeSelectProps["onSearch"]>;
    handleTreeExpand: NonNullable<TreeSelectProps["onTreeExpand"]>;
    isLoading: boolean;
    treeData: TreeNode[];
}

/**
 * Provides a tree of file system items for inputting paths for compression job submission.
 *
 * @return
 */
export const useFileSystemTree = (): FileSystemTree => {
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const [isLoading, setIsLoading] = useState(false);
    const [treeData, setTreeData] = useState<TreeNode[]>([
        {id: "/", value: "/", title: "/", isLeaf: false},
    ]);

    const fetchAndAppendTreeNodes = useCallback(async (path: string): Promise<boolean> => {
        try {
            const fileItems = await listFiles(path);
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

        if (false === treeData.some((node) => "/" === node["id"])) {
            const success = await fetchAndAppendTreeNodes("/");
            if (!success) {
                return false;
            }
        }

        let currentPath = "/";
        for (const segment of pathSegments) {
            const nextPath = "/" === currentPath ?
                `/${segment}` :
                `${currentPath}/${segment}`;

            if (!treeData.some((node) => node["id"] === nextPath)) {
                const success = await fetchAndAppendTreeNodes(currentPath);
                if (!success) {
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
        [fetchAndAppendTreeNodes]
    );

    const handleSearch = useCallback<NonNullable<TreeSelectProps["onSearch"]>>(
        (value) => {
            if (!value.trim()) {
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
        ]
    );

    const handleTreeExpand = useCallback<NonNullable<TreeSelectProps["onTreeExpand"]>>(
        (keys) => {
            setExpandedKeys(keys as string[]);
        },
        []
    );

    return {
        expandedKeys,
        handleLoadData,
        handleSearch,
        handleTreeExpand,
        isLoading,
        treeData,
    };
};

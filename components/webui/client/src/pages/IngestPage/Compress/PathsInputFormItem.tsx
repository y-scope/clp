import {
    useCallback,
    useState,
} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {
    Empty,
    Form,
    GetProp,
    message,
    Spin,
    TreeSelect,
    TreeSelectProps,
} from "antd";

import {listFiles} from "../../../api/os";


type TreeNode = Omit<GetProp<TreeSelectProps, "treeData">[number], "label">;

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
 * @return
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
 * Renders an empty state display when a path is not found.
 *
 * @return
 */
const PathNotFoundEmpty = () => (
    <Empty
        description={"Path not found"}
        image={Empty.PRESENTED_IMAGE_SIMPLE}/>
);

/**
 * Renders an empty state with a loading spinner.
 *
 * @return
 */
const PathLoadingEmpty = () => (
    <Empty
        image={Empty.PRESENTED_IMAGE_SIMPLE}
        description={<Spin
            size={"default"}
            spinning={true}/>}/>
);

/**
 * Renders a form item for inputting file paths for compression job submission.
 *
 * @return
 */
const PathsInputFormItem = () => {
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const [isLoading, setIsLoading] = useState(false);
    const [treeData, setTreeData] = useState<TreeNode[]>([
        {id: "/", value: "/", title: "/", isLeaf: false},
    ]);

    const fetchAndAppendTreeNodes = useCallback(async (path: string): Promise<boolean> => {
        try {
            const {data} = await listFiles(path);
            type FileItem = {isExpandable: boolean; name: string; parentPath: string};

            const newNodes = (data as FileItem[]).map(mapFileToTreeNode);

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

            setExpandedKeys((prev) => Array.from(new Set([...prev,
                path])));

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

        if (!treeData.some((node) => "/" === node["id"])) {
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
    }, [treeData,
        fetchAndAppendTreeNodes]);

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
            // eslint-disable-next-line no-void
            void (async () => {
                try {
                    const basePath = extractBasePath(value);

                    if (expandedKeys.includes(basePath)) {
                        return;
                    }

                    const parentsLoaded = await loadMissingParents(basePath);
                    if (parentsLoaded) {
                        await fetchAndAppendTreeNodes(basePath);
                    }
                } finally {
                    setIsLoading(false);
                }
            })();
        },
        [fetchAndAppendTreeNodes,
            loadMissingParents,
            expandedKeys]
    );

    const handleTreeExpand = useCallback<NonNullable<TreeSelectProps["onTreeExpand"]>>(
        (keys) => {
            setExpandedKeys(keys as string[]);
        },
        []
    );

    return (
        <Form.Item
            label={"Paths"}
            name={"paths"}
            rules={[{required: true, message: "Please select at least one path"}]}
        >
            <TreeSelect
                allowClear={true}
                listHeight={512}
                loadData={handleLoadData}
                multiple={true}
                placeholder={"Please select paths to compress"}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={true}
                treeCheckable={true}
                treeData={treeData}
                treeDataSimpleMode={true}
                treeExpandAction={"click"}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                notFoundContent={isLoading ?
                    <PathLoadingEmpty/> :
                    <PathNotFoundEmpty/>}
                switcherIcon={(props: {expanded?: boolean}) => (
                    props.expanded ?
                        <MinusOutlined style={{color: "grey"}}/> :
                        <PlusOutlined style={{color: "grey"}}/>
                )}
                onSearch={handleSearch}
                onTreeExpand={handleTreeExpand}/>
        </Form.Item>
    );
};


export default PathsInputFormItem;

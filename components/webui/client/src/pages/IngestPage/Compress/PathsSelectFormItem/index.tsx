import {
    useCallback,
    useRef,
    useState,
} from "react";

import {FileEntry} from "@webui/common/schemas/os";
import {
    Form,
    message,
    TreeSelect,
    TreeSelectProps,
} from "antd";

import {listFiles} from "../../../../api/os";
import {
    LIST_HEIGHT_PX,
    ROOT_NODE,
} from "../typings";
import {
    addServerPrefix,
    toTreeNode,
    type TreeNode,
} from "../utils";
import SwitcherIcon from "./SwitcherIcon";


type LoadDataNode = Parameters<NonNullable<TreeSelectProps["loadData"]>>[0];

type TreeExpandKeys = Parameters<NonNullable<TreeSelectProps["onTreeExpand"]>>[0];

/**
 * Form item with TreeSelect for selecting file paths for compression.
 * Supports lazy loading and search with auto-expand.
 *
 * @return
 */
const PathsSelectFormItem = () => {
    const [treeData, setTreeData] = useState<TreeNode[]>([ROOT_NODE]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);

    // Use a ref, instead of a state passed to AntD's `treeLoadedKeys`, to dedupe load requests.
    const loadedPathsRef = useRef(new Set<string>());

    const addNodes = useCallback((nodes: TreeNode[]) => {
        setTreeData((prev) => {
            const existingIds = new Set(prev.map((n) => n["id"] as string));
            const newNodes = nodes.filter((n) => false === existingIds.has(n["id"] as string));

            return 0 < newNodes.length ?
                [
                    ...prev,
                    ...newNodes,
                ] :
                prev;
        });
    }, []);

    const loadPath = useCallback(async (path: string) => {
        if (loadedPathsRef.current.has(path)) {
            return;
        }
        loadedPathsRef.current.add(path);
        try {
            const files: FileEntry[] = await listFiles(addServerPrefix(path));
            addNodes(files.map((f) => toTreeNode(f, path)));
        } catch (e) {
            loadedPathsRef.current.delete(path);
            throw e;
        }
    }, [addNodes]);

    const handleLoadData = useCallback(async ({value}: LoadDataNode) => {
        if ("string" !== typeof value) {
            return;
        }
        try {
            await loadPath(value);
        } catch (e) {
            console.error("Failed to load directory:", e);
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while loading paths");
        }
    }, [loadPath]);

    const handleTreeExpand = useCallback((keys: TreeExpandKeys) => {
        setExpandedKeys(keys as string[]);
    }, []);

    return (
        <Form.Item
            label={"Paths"}
            name={"paths"}
            rules={[{required: true, message: "Please select at least one path"}]}
        >
            <TreeSelect
                allowClear={true}
                listHeight={LIST_HEIGHT_PX}
                loadData={handleLoadData}
                multiple={true}
                placeholder={"Select paths to compress"}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={false}
                switcherIcon={SwitcherIcon}

                treeCheckable={true}
                treeData={treeData}
                treeDataSimpleMode={true}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                onTreeExpand={handleTreeExpand}/>
        </Form.Item>
    );
};


export default PathsSelectFormItem;

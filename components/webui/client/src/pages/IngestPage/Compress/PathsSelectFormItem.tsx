import React, {
    useCallback,
    useRef,
    useState,
} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {FileEntry} from "@webui/common/schemas/os";
import {
    Form,
    message,
    TreeSelect,
} from "antd";

import {listFiles} from "../../../api/os";
import {
    LIST_HEIGHT_PX,
    ROOT_NODE,
} from "./typings";
import {
    addServerPrefix,
    toTreeNode,
    type TreeNode,
} from "./utils";


/**
 * Icon component for tree node expand/collapse state.
 *
 * @param props Component props.
 * @param props.expanded Whether the node is expanded (passed by Ant Design).
 * @return
 */
// The "expanded" prop name is dictated by Ant Design TreeSelect.
// eslint-disable-next-line react/boolean-prop-naming
const SwitcherIcon = ({expanded}: {expanded?: boolean}) => (expanded ?
    <MinusOutlined style={{color: "grey"}}/> :
    <PlusOutlined style={{color: "grey"}}/>);

/**
 * Form item with TreeSelect for selecting file paths for compression.
 * Supports lazy loading and search with auto-expand.
 *
 * @return
 */
const PathsSelectFormItem = () => {
    const [treeData, setTreeData] = useState<TreeNode[]>([ROOT_NODE]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const loadedPaths = useRef(new Set<string>());

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
        if (loadedPaths.current.has(path)) {
            return;
        }
        loadedPaths.current.add(path);
        try {
            const files: FileEntry[] = await listFiles(addServerPrefix(path));
            addNodes(files.map((f) => toTreeNode(f, path)));
        } catch (e) {
            loadedPaths.current.delete(path);
            throw e;
        }
    }, [addNodes]);

    const handleLoadData = useCallback(async ({value}: {value?: string | number}) => {
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

    const handleTreeExpand = useCallback((keys: React.Key[]) => {
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

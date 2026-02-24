import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {FileEntry} from "@webui/common/schemas/os";
import {Form} from "antd";

import {listFiles} from "../../../../api/os";
import DirectoryTreeSelect from "./DirectoryTreeSelect";
import {
    ROOT_NODE,
    ROOT_PATH,
    TreeNode,
} from "./typings";
import {
    addServerPrefix,
    handleLoadError,
    toTreeNode,
} from "./utils";


/**
 * Form item with TreeSelect for selecting file paths for compression.
 * Uses DirectoryTree for intuitive folder/file navigation.
 *
 * @return
 * @see https://ant.design/components/tree#tree-demo-directory
 */
const PathsSelectFormItem = () => {
    const form = Form.useFormInstance();
    const [treeData, setTreeData] = useState<TreeNode[]>([ROOT_NODE]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const [checkedKeys, setCheckedKeys] = useState<string[]>([]);

    // Use a ref, instead of a state passed to AntD's `treeLoadedKeys`, to dedupe load requests.
    const loadedPathsRef = useRef(new Set<string>());

    const addNodes = useCallback((nodes: TreeNode[]) => {
        setTreeData((prev) => {
            const existingIds = new Set(prev.map((n) => n.id));
            const newNodes = nodes.filter((n) => false === existingIds.has(n.id));

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

    const handleCheck = useCallback((keys: string[]) => {
        setCheckedKeys(keys);
        form.setFieldValue("paths", keys);
    }, [form]);

    const handleExpand = useCallback((keys: string[]) => {
        setExpandedKeys(keys);
    }, []);

    const handleLoadData = useCallback(async (path: string) => {
        await loadPath(path).catch(handleLoadError);
    }, [loadPath]);

    // On initialization, load root directory contents.
    useEffect(() => {
        loadPath(ROOT_PATH)
            .then(() => {
                setExpandedKeys([ROOT_PATH]);
            })
            .catch(handleLoadError);
    }, [loadPath]);

    return (
        <Form.Item
            label={"Paths"}
            name={"paths"}
            rules={[{required: true, message: "Please select at least one path"}]}
        >
            <DirectoryTreeSelect
                checkedKeys={checkedKeys}
                expandedKeys={expandedKeys}
                treeData={treeData}
                onCheck={handleCheck}
                onExpand={handleExpand}
                onLoadData={handleLoadData}/>
        </Form.Item>
    );
};


export default PathsSelectFormItem;

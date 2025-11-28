import {
    useCallback,
    useEffect,
} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {
    Empty,
    Form,
    Spin,
    TreeSelect,
} from "antd";
import {DataNode} from "antd/es/tree";

import useFileSystemTreeStore from "./fileSystemTreeStore";
import styles from "./PathsSelectFormItem.module.css";
import {
    handleLoadData,
    handleSearch,
    initializeTree,
} from "./treeEventHandlers";


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
 * Renders a form item for selecting file paths for compression job submission.
 *
 * @return
 */
const PathsSelectFormItem = () => {
    const expandedKeys = useFileSystemTreeStore((state) => state.expandedKeys);
    const isLoading = useFileSystemTreeStore((state) => state.isLoading);
    const treeData = useFileSystemTreeStore((state) => state.treeData);

    useEffect(() => {
        initializeTree();
    }, []);

    const handleTitleClick = useCallback((e: React.MouseEvent, node: DataNode) => {
        e.stopPropagation();
        const nodeKey = node.key as string;
        const {setExpandedKeys} = useFileSystemTreeStore.getState();
        const currentExpandedKeys = useFileSystemTreeStore.getState().expandedKeys;
        const isExpanded = currentExpandedKeys.includes(nodeKey);

        if (isExpanded) {
            setExpandedKeys(currentExpandedKeys.filter((key) => key !== nodeKey));
        } else {
            setExpandedKeys([...currentExpandedKeys,
                nodeKey]);
        }
    }, []);

    const treeTitleRender = useCallback((node: DataNode) => (
        <span
            className={styles["treeNodeTitle"]}
            onClick={(e) => {
                handleTitleClick(e, node);
            }}
        >
            {node.title as string}
        </span>
    ), [handleTitleClick]);

    return (
        <Form.Item
            label={"Paths"}
            name={"paths"}
            rules={[{required: true, message: "Please select at least one path"}]}
        >
            <TreeSelect
                allowClear={true}
                filterTreeNode={false}
                listHeight={512}
                loadData={handleLoadData}
                multiple={true}
                placeholder={"Please select paths to compress"}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={true}
                treeCheckable={true}
                treeData={treeData}
                treeDataSimpleMode={true}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                treeTitleRender={treeTitleRender}
                notFoundContent={isLoading ?
                    <PathLoadingEmpty/> :
                    <PathNotFoundEmpty/>}
                switcherIcon={(props: {expanded?: boolean}) => (
                    props.expanded ?
                        <MinusOutlined style={{color: "grey"}}/> :
                        <PlusOutlined style={{color: "grey"}}/>
                )}
                onSearch={handleSearch}
                onTreeExpand={(keys) => {
                    useFileSystemTreeStore.getState().setExpandedKeys(keys as string[]);
                }}/>
        </Form.Item>
    );
};


export default PathsSelectFormItem;

import React, {
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
import {SafeKey} from "antd/es/table/interface";
import {DataNode} from "antd/es/tree";

import useFileSystemTreeStore from "./fileSystemTreeStore";
import styles from "./PathsSelectFormItem.module.css";
import {
    handleLoadData,
    handleSearch,
    initializeTree,
} from "./treeEventHandlers";


const TREE_SELECT_LIST_HEIGHT = 512;

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

    const handleTitleClick = useCallback((ev: React.MouseEvent, node: DataNode) => {
        ev.stopPropagation();
        const {toggleNode} = useFileSystemTreeStore.getState();
        toggleNode(node.key as string);
    }, []);

    const handleTreeExpand = useCallback((newExpandedKeys: SafeKey[]) => {
        const {handleTreeExpansion} = useFileSystemTreeStore.getState();
        handleTreeExpansion(newExpandedKeys as string[]);
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
                listHeight={TREE_SELECT_LIST_HEIGHT}
                loadData={handleLoadData}
                multiple={true}
                placeholder={"Please select paths to compress"}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={true}
                treeCheckable={true}
                treeData={treeData as DataNode[]}
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
                onTreeExpand={handleTreeExpand}/>
        </Form.Item>
    );
};


export default PathsSelectFormItem;

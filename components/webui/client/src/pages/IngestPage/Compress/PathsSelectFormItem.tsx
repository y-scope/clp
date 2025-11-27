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

import styles from "./PathsSelectFormItem.module.css";
import useFileSystemTree from "./useFileSystemTree";


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
    const {
        expandedKeys,
        isLoading,
        handleLoadData,
        handleSearch,
        handleTreeExpand,
        handleTitleClick,
        treeData,
    } = useFileSystemTree();

    const treeDataWithClickHandlers = treeData.map((node) => ({
        ...node,
        title: (
            <span
                className={styles["treeNodeTitle"]}
                onClick={(e) => {
                    e.stopPropagation();
                    const key = node["value"] as string;
                    const expanded = expandedKeys.includes(key);
                    handleTitleClick(key, expanded);
                }}
            >
                {node["title"]}
            </span>
        ),
    }));

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
                treeData={treeDataWithClickHandlers}
                treeDataSimpleMode={true}
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


export default PathsSelectFormItem;

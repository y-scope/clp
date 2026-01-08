import React, {
    useCallback,
    useEffect,
    useMemo,
    useState,
} from "react";

import {
    Tree,
    TreeSelect,
} from "antd";
import type {TreeProps} from "antd/es";

import styles from "./index.module.css";
import {TreeNode} from "./typings";
import {
    flatToHierarchy,
    getListHeight,
} from "./utils";


const {DirectoryTree} = Tree;


interface DirectoryTreeSelectProps {
    expandedKeys: string[];
    treeData: TreeNode[];
    onChange: (values: string[]) => void;
    onDataLoad: (path: string) => Promise<void>;
    onExpand: (keys: string[]) => void;
}

/**
 * TreeSelect component that uses DirectoryTree for the popup. Provides folder/file icons and
 * intuitive directory navigation.
 *
 * @param props
 * @param props.expandedKeys
 * @param props.treeData
 * @param props.onChange
 * @param props.onDataLoad
 * @param props.onExpand
 * @return
 * @see https://ant.design/components/tree#tree-demo-directory
 */
const DirectoryTreeSelect = ({
    expandedKeys,
    treeData,
    onChange,
    onDataLoad,
    onExpand,
}: DirectoryTreeSelectProps) => {
    const [checkedKeys, setCheckedKeys] = useState<string[]>([]);
    const [listHeight, setListHeight] = useState<number>(getListHeight);

    const hierarchicalTreeData = useMemo(
        () => flatToHierarchy(treeData),
        [treeData]
    );

    const handleTreeSelectDataLoad = useCallback(async ({value: nodeValue}: {value?: unknown}) => {
        if ("string" !== typeof nodeValue) {
            return;
        }
        await onDataLoad(nodeValue);
    }, [onDataLoad]);

    const handleDirectoryTreeLoadData = useCallback(async (node: {key: React.Key}) => {
        await onDataLoad(node.key as string);
    }, [onDataLoad]);

    const handleTreeExpand = useCallback((keys: React.Key[]) => {
        onExpand(keys as string[]);
    }, [onExpand]);

    const handleDirectoryTreeCheck: TreeProps["onCheck"] = useCallback((
        checked: React.Key[] | {checked: React.Key[]; halfChecked: React.Key[]}
    ) => {
        const keys = Array.isArray(checked) ?
            checked :
            checked.checked;

        const stringKeys = keys as string[];
        setCheckedKeys(stringKeys);
        onChange(stringKeys);
    }, [onChange]);

    const handleTreeSelectChange = useCallback((values: string[]) => {
        setCheckedKeys(values);
        onChange(values);
    }, [onChange]);

    const renderPopup = useCallback(() => (
        <div
            className={styles["directoryTreePopup"]}
            style={{height: listHeight, overflow: "auto"}}
        >
            <DirectoryTree
                checkable={true}
                checkedKeys={checkedKeys}
                expandedKeys={expandedKeys}
                loadData={handleDirectoryTreeLoadData}
                treeData={hierarchicalTreeData}
                onCheck={handleDirectoryTreeCheck}
                onExpand={handleTreeExpand}/>
        </div>
    ), [
        checkedKeys,
        expandedKeys,
        handleDirectoryTreeCheck,
        handleDirectoryTreeLoadData,
        handleTreeExpand,
        hierarchicalTreeData,
        listHeight,
    ]);

    // On initialization, set up window resize listener to adjust list height.
    useEffect(() => {
        const handleResize = () => {
            setListHeight(getListHeight());
        };

        window.addEventListener("resize", handleResize);

        return () => {
            window.removeEventListener("resize", handleResize);
        };
    }, []);

    return (
        <TreeSelect
            allowClear={true}
            listHeight={listHeight}
            loadData={handleTreeSelectDataLoad}
            multiple={true}
            placeholder={"Select paths to compress"}
            popupRender={renderPopup}
            showCheckedStrategy={TreeSelect.SHOW_PARENT}
            showSearch={false}
            treeCheckable={true}
            treeData={treeData}
            treeDataSimpleMode={true}
            treeExpandedKeys={expandedKeys}
            treeNodeLabelProp={"value"}
            value={checkedKeys}
            onChange={handleTreeSelectChange}
            onTreeExpand={handleTreeExpand}/>
    );
};


export default DirectoryTreeSelect;

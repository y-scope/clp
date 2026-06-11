import {
    useCallback,
    useMemo,
} from "react";

import {Select} from "antd";
import type {TreeDataNode} from "antd/es";

import DirectoryTreePopup from "./DirectoryTreePopup";
import {TreeNode} from "./typings";
import {
    filterToParents,
    flatToHierarchy,
    removeWithDescendants,
} from "./utils";


interface DirectoryTreeSelectProps {
    checkedKeys: string[];
    expandedKeys: string[];
    treeData: TreeNode[];
    onCheck: (keys: string[]) => void;
    onExpand: (keys: string[]) => void;
    onLoadData: (path: string) => Promise<void>;
}

/**
 * Renders a select component that uses DirectoryTree for the dropdown. Provides folder/file
 * icons and intuitive directory navigation.
 *
 * @param props
 * @param props.checkedKeys
 * @param props.expandedKeys
 * @param props.treeData
 * @param props.onCheck
 * @param props.onExpand
 * @param props.onLoadData
 * @return
 */
const DirectoryTreeSelect = ({
    checkedKeys,
    expandedKeys,
    treeData,
    onCheck,
    onExpand,
    onLoadData,
}: DirectoryTreeSelectProps) => {
    const hierarchicalTreeData: TreeDataNode[] = useMemo(
        () => flatToHierarchy(treeData),
        [treeData]
    );
    const selectOptions = useMemo(
        () => treeData.map((node) => ({label: node.value, value: node.value})),
        [treeData]
    );
    const displayValue = useMemo(
        () => filterToParents(treeData, checkedKeys),
        [
            checkedKeys,
            treeData,
        ]
    );

    const handleClear = useCallback(() => {
        onCheck([]);
    }, [onCheck]);

    const handleDeselect = useCallback((value: string) => {
        onCheck(removeWithDescendants(treeData, checkedKeys, value));
    }, [
        checkedKeys,
        onCheck,
        treeData,
    ]);

    const renderDropdown = useCallback(() => (
        <DirectoryTreePopup
            checkedKeys={checkedKeys}
            expandedKeys={expandedKeys}
            treeData={hierarchicalTreeData}
            onCheck={onCheck}
            onExpand={onExpand}
            onLoadData={onLoadData}/>
    ), [
        checkedKeys,
        expandedKeys,
        hierarchicalTreeData,
        onCheck,
        onExpand,
        onLoadData,
    ]);

    return (
        <Select
            allowClear={true}
            mode={"multiple"}
            options={selectOptions}
            placeholder={"Select paths to compress"}
            popupRender={renderDropdown}
            value={displayValue}
            onClear={handleClear}
            onDeselect={handleDeselect}/>
    );
};


export default DirectoryTreeSelect;

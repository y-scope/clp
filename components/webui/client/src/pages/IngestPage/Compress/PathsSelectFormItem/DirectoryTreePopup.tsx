import React, {
    useCallback,
    useEffect,
    useState,
} from "react";

import {Tree} from "antd";
import type {TreeDataNode} from "antd/es";
import type {TreeProps} from "antd/es/tree";

import styles from "./index.module.css";
import {getListHeight} from "./utils";


const {DirectoryTree} = Tree;


interface DirectoryTreePopupProps {
    checkedKeys: string[];
    expandedKeys: string[];
    treeData: TreeDataNode[];
    onCheck: (keys: string[]) => void;
    onExpand: (keys: string[]) => void;
    onLoadData: (path: string) => Promise<void>;
}

/**
 * Renders a popup component containing a DirectoryTree for path selection.
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
const DirectoryTreePopup = ({
    checkedKeys,
    expandedKeys,
    treeData,
    onCheck,
    onExpand,
    onLoadData,
}: DirectoryTreePopupProps) => {
    const [height, setHeight] = useState<number>(getListHeight);

    const handleCheck: TreeProps["onCheck"] = useCallback((
        checked: React.Key[] | {checked: React.Key[]; halfChecked: React.Key[]}
    ) => {
        const keys = Array.isArray(checked) ?
            checked :
            checked.checked;

        onCheck(keys as string[]);
    }, [onCheck]);

    const handleExpand = useCallback((keys: React.Key[]) => {
        onExpand(keys as string[]);
    }, [onExpand]);

    const handleLoadData = useCallback(async (node: TreeDataNode) => {
        await onLoadData(node.key as string);
    }, [onLoadData]);

    useEffect(() => {
        const handleResize = () => {
            setHeight(getListHeight());
        };

        window.addEventListener("resize", handleResize);

        return () => {
            window.removeEventListener("resize", handleResize);
        };
    }, []);

    return (
        <div
            className={styles["directoryTreePopup"]}
            style={{height: height, overflow: "auto"}}
        >
            <DirectoryTree
                checkable={true}
                checkedKeys={checkedKeys}
                expandedKeys={expandedKeys}
                loadData={handleLoadData}
                treeData={treeData}
                onCheck={handleCheck}
                onExpand={handleExpand}/>
        </div>
    );
};


export default DirectoryTreePopup;

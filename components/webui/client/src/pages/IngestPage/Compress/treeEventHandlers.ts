import React from "react";

import {TreeSelectProps} from "antd";

import useFileSystemTreeStore from "./fileSystemTreeStore";
import {extractBasePath} from "./utils";


/**
 * Handles loading data for a tree node.
 *
 * @param node
 */
const handleLoadData: NonNullable<TreeSelectProps["loadData"]> = async (node) => {
    const {fetchAndAppendTreeNodes, setIsLoading} = useFileSystemTreeStore.getState();
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
};

/**
 * Handles search input in the tree select.
 *
 * @param value
 */
const handleSearch: NonNullable<TreeSelectProps["onSearch"]> = (value) => {
    if (0 === value.trim().length) {
        return;
    }

    const {
        expandedKeys,
        fetchAndAppendTreeNodes,
        loadMissingParents,
        setIsLoading,
    } = useFileSystemTreeStore.getState();

    setIsLoading(true);
    (async () => {
        try {
            const basePath = extractBasePath(value);
            const parentsLoaded = await loadMissingParents(basePath);
            if (parentsLoaded && false === expandedKeys.includes(basePath)) {
                await fetchAndAppendTreeNodes(basePath);
            }
        } finally {
            setIsLoading(false);
        }
    })().catch((e: unknown) => {
        console.error("Failed to load missing parents", e);
    });
};

/**
 * Handles tree node expansion.
 *
 * @param keys
 */
const handleTreeExpand: NonNullable<TreeSelectProps["onTreeExpand"]> = (keys) => {
    const {
        fetchAndAppendTreeNodes,
        setExpandedKeys,
        setIsLoading,
    } = useFileSystemTreeStore.getState();

    setExpandedKeys(keys as string[]);
    const lastKey = keys[keys.length - 1] as string;

    setIsLoading(true);
    fetchAndAppendTreeNodes(lastKey)
        .catch((e: unknown) => {
            console.error("Failed to load expanded path", e);
        })
        .finally(() => {
            setIsLoading(false);
        });
};

/**
 * Creates a click handler for a tree node title.
 *
 * @param nodeValue
 * @param expandedKeys
 * @return Click event handler
 */
const createTitleClickHandler = (
    nodeValue: string,
    expandedKeys: string[]
) => (e: React.MouseEvent) => {
    const {toggleNodeExpansion} = useFileSystemTreeStore.getState();
    e.stopPropagation();
    const expanded = expandedKeys.includes(nodeValue);
    toggleNodeExpansion(nodeValue, expanded).catch((error: unknown) => {
        console.error("Failed to toggle node expansion", error);
    });
};

/**
 * Initializes the file system tree by loading the root directory.
 */
const initializeTree = () => {
    const {fetchAndAppendTreeNodes} = useFileSystemTreeStore.getState();
    fetchAndAppendTreeNodes("/").catch((e: unknown) => {
        console.error("Failed to load root directory", e);
    });
};


export {
    createTitleClickHandler,
    handleLoadData,
    handleSearch,
    handleTreeExpand,
    initializeTree,
};

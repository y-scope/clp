import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {S3ListRequest} from "@webui/common/schemas/s3";
import {message} from "antd";

import {listS3Objects} from "../../../../api/s3";
import {
    ROOT_ID,
    ROOT_NODE,
    ROOT_PREFIX,
    S3TreeNode,
} from "./typings";
import {toTreeNodes} from "./utils";


const LOAD_MORE_PREFIX = "__load_more__";

/**
 * Creates a "Load more..." pagination node.
 *
 * @param prefix The parent prefix this node belongs to.
 * @param token The continuation token for the next page.
 * @return A pagination tree node.
 */
const createLoadMoreNode = (prefix: string, token: string): S3TreeNode => {
    const parentId = ROOT_PREFIX === prefix ?
        ROOT_ID :
        prefix;

    return {
        continuationToken: token,
        id: `${LOAD_MORE_PREFIX}${prefix}::${token}`,
        isLeaf: true,
        isLoadMore: true,
        isPrefix: false,
        pId: parentId,
        parentPrefix: prefix,
        title: "Load more...",
        value: `${LOAD_MORE_PREFIX}${prefix}::${token}`,
    };
};

interface S3ConnectionConfig {
    bucket: string | undefined;
    endpointUrl: string | undefined;
    regionCode: string | undefined;
}

/**
 * Builds S3 list request params from connection config.
 *
 * @param config S3 connection config.
 * @param prefix S3 key prefix to list.
 * @param token Optional continuation token.
 * @return S3 list request params.
 */
const buildListParams = (
    config: S3ConnectionConfig,
    prefix: string,
    token?: string,
): S3ListRequest => {
    const params: S3ListRequest = {
        bucket: config.bucket as string,
        prefix: prefix,
    };

    if (token) {
        params.continuationToken = token;
    }
    if (config.regionCode) {
        params.regionCode = config.regionCode;
    }
    if (config.endpointUrl) {
        params.endpointUrl = config.endpointUrl;
    }

    return params;
};

/**
 * Manages the tree node state (add, remove, reset).
 *
 * @param config S3 connection config for resetting on change.
 * @return Tree data state and mutators.
 */
const useTreeData = (config: S3ConnectionConfig) => {
    const [treeData, setTreeData] = useState<S3TreeNode[]>([ROOT_NODE]);
    const loadedRef = useRef(new Set<string>());

    useEffect(() => {
        setTreeData([ROOT_NODE]);
        loadedRef.current.clear();
    }, [
        config.bucket,
        config.regionCode,
        config.endpointUrl,
    ]);

    const addNodes = useCallback((nodes: S3TreeNode[]) => {
        setTreeData((prev) => {
            const ids = new Set(prev.map((n) => n.id));
            const fresh = nodes.filter((n) => false === ids.has(n.id));

            return 0 < fresh.length ?
                [...prev,
                    ...fresh] :
                prev;
        });
    }, []);

    const removeNode = useCallback((nodeId: string) => {
        setTreeData((prev) => prev.filter((n) => n.id !== nodeId));
    }, []);

    return {
        addNodes: addNodes,
        loadedRef: loadedRef,
        removeNode: removeNode,
        treeData: treeData,
    };
};

/**
 * Creates a memoized function to load an S3 prefix into the tree.
 *
 * @param config S3 connection config.
 * @param addNodes Function to add nodes to the tree.
 * @param loadedRef Ref tracking loaded cache keys.
 * @return The loadPrefix callback.
 */
const useLoadPrefix = (
    config: S3ConnectionConfig,
    addNodes: (nodes: S3TreeNode[]) => void,
    loadedRef: React.RefObject<Set<string>>,
) => {
    const {bucket, endpointUrl, regionCode} = config;

    return useCallback(async (prefix: string, token?: string) => {
        if (!bucket) {
            return;
        }
        const cacheKey = `${prefix}::${token ?? ""}`;
        if (loadedRef.current.has(cacheKey)) {
            return;
        }
        loadedRef.current.add(cacheKey);

        try {
            const cfg: S3ConnectionConfig = {
                bucket: bucket,
                endpointUrl: endpointUrl,
                regionCode: regionCode,
            };
            const params = buildListParams(cfg, prefix, token);
            const resp = await listS3Objects(params);

            addNodes(toTreeNodes(resp.entries, prefix));
            if (resp.isTruncated && resp.nextContinuationToken) {
                addNodes([createLoadMoreNode(prefix, resp.nextContinuationToken)]);
            }
        } catch (e) {
            loadedRef.current.delete(cacheKey);
            throw e;
        }
    }, [
        bucket,
        regionCode,
        endpointUrl,
        addNodes,
        loadedRef,
    ]);
};

/**
 * Manages S3 tree state: loading, pagination, and reset on config changes.
 *
 * @param config S3 connection config.
 * @return Tree state and handlers.
 */
const useS3Tree = (config: S3ConnectionConfig) => {
    const {addNodes, loadedRef, removeNode, treeData} = useTreeData(config);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const loadPrefix = useLoadPrefix(config, addNodes, loadedRef);

    useEffect(() => {
        setExpandedKeys([]);
    }, [
        config.bucket,
        config.regionCode,
        config.endpointUrl,
    ]);

    useEffect(() => {
        if (!config.bucket) {
            return;
        }
        loadPrefix(ROOT_PREFIX)
            .then(() => {
                setExpandedKeys([ROOT_ID]);
            })
            .catch((e: unknown) => {
                console.error("Failed to load S3 bucket root:", e);
                message.error(e instanceof Error ?
                    e.message :
                    "Failed to list S3 objects");
            });
    }, [
        config.bucket,
        loadPrefix,
    ]);

    const handleLoadMore = useCallback((node: S3TreeNode) => {
        if ("string" === typeof node.parentPrefix && node.continuationToken) {
            removeNode(node.id);
            loadPrefix(node.parentPrefix, node.continuationToken)
                .catch((e: unknown) => {
                    console.error("Failed to load more:", e);
                    message.error(e instanceof Error ?
                        e.message :
                        "Failed to load more S3 objects");
                });
        }
    }, [
        loadPrefix,
        removeNode,
    ]);

    return {
        expandedKeys: expandedKeys,
        handleLoadMore: handleLoadMore,
        loadPrefix: loadPrefix,
        setExpandedKeys: setExpandedKeys,
        treeData: treeData,
    };
};

export type {S3ConnectionConfig};
export {LOAD_MORE_PREFIX};
export default useS3Tree;

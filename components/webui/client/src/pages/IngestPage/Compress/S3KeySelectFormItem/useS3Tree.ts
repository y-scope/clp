import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {S3ListRequest} from "@webui/common/schemas/s3";
import {AxiosError} from "axios";

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
 * Extracts a human-readable error message from an S3 API error.
 *
 * @param e
 * @param fallback
 * @return
 */
const getErrorMessage = (e: unknown, fallback: string): string => {
    if (e instanceof AxiosError) {
        const data: unknown = e.response?.data;
        if ("object" === typeof data && null !== data && "message" in data) {
            const {message} = data as {message: unknown};
            if ("string" === typeof message) {
                return message;
            }
        }
        if ("string" === typeof data) {
            return data;
        }
    }

    return e instanceof Error ?
        e.message :
        fallback;
};


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
        label: "Load more...",
        pId: parentId,
        parentPrefix: prefix,
        title: "Load more...",
        value: `${LOAD_MORE_PREFIX}${prefix}::${token}`,
    };
};

/**
 * Manages the tree node state (add, remove, reset).
 *
 * @param bucket S3 bucket name for resetting on change.
 * @param regionCode
 * @return Tree data state and mutators.
 */
const useTreeData = (bucket: string | undefined, regionCode: string | undefined) => {
    const [treeData, setTreeData] = useState<S3TreeNode[]>([]);
    const loadedRef = useRef(new Set<string>());

    useEffect(() => {
        setTreeData([]);
        loadedRef.current.clear();
    }, [bucket,
        regionCode]);

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
 * @param bucket S3 bucket name.
 * @param regionCode
 * @param addNodes Function to add nodes to the tree.
 * @param loadedRef Ref tracking loaded cache keys.
 * @return The loadPrefix callback.
 */
const useLoadPrefix = (
    bucket: string | undefined,
    regionCode: string | undefined,
    addNodes: (nodes: S3TreeNode[]) => void,
    loadedRef: React.RefObject<Set<string>>,
) => {
    return useCallback(async (prefix: string, token?: string) => {
        if (!bucket || !regionCode) {
            return;
        }
        const cacheKey = `${prefix}::${token ?? ""}`;
        if (loadedRef.current.has(cacheKey)) {
            return;
        }
        loadedRef.current.add(cacheKey);

        try {
            const params: S3ListRequest = {
                bucket: bucket,
                prefix: prefix,
                region: regionCode,
            };

            if (token) {
                params.continuationToken = token;
            }

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
        addNodes,
        loadedRef,
    ]);
};

/**
 * Manages S3 tree state: loading, pagination, and reset on bucket change.
 *
 * @param bucket S3 bucket name.
 * @param regionCode
 * @param onError Called with an error message on failure, or `null` to clear.
 * @return Tree state and handlers.
 */
const useS3Tree = (
    bucket: string | undefined,
    regionCode: string | undefined,
    onError?: (msg: string | null) => void,
) => {
    const {addNodes, loadedRef, removeNode, treeData} = useTreeData(bucket, regionCode);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const loadPrefix = useLoadPrefix(bucket, regionCode, addNodes, loadedRef);

    const onErrorRef = useRef(onError);
    onErrorRef.current = onError;

    useEffect(() => {
        setExpandedKeys([]);
    }, [bucket,
        regionCode]);

    useEffect(() => {
        onErrorRef.current?.(null);
        if (!bucket || !regionCode) {
            return;
        }
        addNodes([ROOT_NODE]);
        setExpandedKeys([ROOT_ID]);
        loadPrefix(ROOT_PREFIX)
            .catch((e: unknown) => {
                console.error("Failed to load S3 bucket root:", e);
                onErrorRef.current?.(getErrorMessage(e, "S3 bucket or region not found."));
            });
    }, [
        bucket,
        regionCode,
        loadPrefix,
        addNodes,
    ]);

    const handleLoadMore = useCallback((node: S3TreeNode) => {
        if ("string" === typeof node.parentPrefix && node.continuationToken) {
            removeNode(node.id);
            loadPrefix(node.parentPrefix, node.continuationToken)
                .catch((e: unknown) => {
                    console.error("Failed to load more:", e);
                    onErrorRef.current?.(getErrorMessage(e, "Failed to load more S3 objects"));
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

export {
    getErrorMessage,
    LOAD_MORE_PREFIX,
};
export default useS3Tree;

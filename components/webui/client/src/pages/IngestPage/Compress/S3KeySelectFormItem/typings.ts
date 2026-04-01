/**
 * Tree node for Ant Design TreeSelect in simple mode (treeDataSimpleMode).
 */
interface S3TreeNode {
    id: string;
    isLeaf: boolean;
    pId: string;
    title: string;
    value: string;

    /**
     * If true, this is a "Load more..." pagination node, not a real S3 entry.
     */
    isLoadMore?: boolean;

    /**
     * Continuation token for paginating the parent prefix.
     */
    continuationToken?: string;

    /**
     * The parent prefix this load-more node belongs to.
     */
    parentPrefix?: string;

    /**
     * Whether this node represents an S3 common prefix (folder) vs an object (file).
     */
    isPrefix?: boolean;
}

/**
 * The root prefix for S3 listings (bucket root).
 */
const ROOT_PREFIX = "";

/**
 * Node ID for the root of the tree.
 */
const ROOT_ID = "/";

/**
 * Root tree node representing the bucket root.
 */
const ROOT_NODE: S3TreeNode = Object.freeze({
    id: ROOT_ID,
    isLeaf: false,
    isPrefix: true,
    pId: "",
    title: "/  (bucket root)",
    value: ROOT_PREFIX,
});

export type {S3TreeNode};
export {
    ROOT_ID,
    ROOT_NODE,
    ROOT_PREFIX,
};

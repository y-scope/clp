/**
 * Tree node for Ant Design TreeSelect in simple mode (treeDataSimpleMode).
 */
interface TreeNode {
    id: string;
    isLeaf: boolean;
    pId: string | null;
    title: string;
    value: string;
}

const ROOT_PATH = "/";

/**
 * Root tree node representing the filesystem root.
 */
const ROOT_NODE: TreeNode = Object.freeze({
    id: ROOT_PATH,
    isLeaf: false,
    pId: null,
    title: ROOT_PATH,
    value: ROOT_PATH,
});

export type {TreeNode};
export {
    ROOT_NODE,
    ROOT_PATH,
};

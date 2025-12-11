import {type TreeNode} from "./utils";


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


export {
    ROOT_NODE,
    ROOT_PATH,
};

import {type TreeNode} from "./utils";


/**
 * Height of the dropdown list in pixels.
 */
const LIST_HEIGHT_PX = 512;

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
    LIST_HEIGHT_PX,
    ROOT_NODE,
    ROOT_PATH,
};

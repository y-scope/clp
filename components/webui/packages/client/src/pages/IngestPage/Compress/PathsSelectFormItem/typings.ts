/**
 * Tree node for Ant Design TreeSelect in simple mode (treeDataSimpleMode).
 */
interface TreeNode {
    /**
     * Path shown to the user, including the logs-input directory. Matches the original path the
     * Jobs table displays for the resulting compression job.
     */
    displayPath: string;

    id: string;
    isLeaf: boolean;
    pId: string | null;
    title: string;

    /**
     * Path submitted to the API server, relative to the configured logs-input root.
     */
    value: string;
}

const ROOT_PATH = "/";

export type {TreeNode};
export {ROOT_PATH};

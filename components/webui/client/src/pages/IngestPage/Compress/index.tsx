import {
    useCallback,
    useState,
} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {
    Button,
    Empty,
    Form,
    GetProp,
    Input,
    message,
    Spin,
    TreeSelect,
    TreeSelectProps,
    Typography,
} from "antd";

import {submitCompressionJob} from "../../../api/compress";
import {listFiles} from "../../../api/os";
import {DashboardCard} from "../../../components/DashboardCard";
import {settings} from "../../../settings";


/**
 * Maps file system item to Antd TreeSelect flat tree node format.
 *
 * @param props
 * @param props.isExpandable
 * @param props.name
 * @param props.parentPath
 * @return the mapped Antd TreeSelect flat tree node.
 */
const mapFileToTreeNode = ({
    isExpandable,
    name,
    parentPath,
}: {
    isExpandable: boolean;
    name: string;
    parentPath: string;
}) => {
    if (0 === parentPath.length) {
        parentPath = "/";
    }
    const pathPrefix = parentPath.endsWith("/") ?
        parentPath :
        `${parentPath}/`;
    const fullPath = pathPrefix + name;

    return {
        id: fullPath,
        isLeaf: !isExpandable,
        pId: parentPath,
        title: name,
        value: fullPath,
    };
};

type TreeNode = Omit<GetProp<TreeSelectProps, "treeData">[number], "label">;


type FormValues = {
    paths: string[];
    dataset?: string;
    timestampKey?: string;
};

/**
 * Renders an empty state display when a path is not found.
 *
 * @return
 */
const PathNotFoundEmpty = () => (
    <Empty
        description={"Path not found"}
        image={Empty.PRESENTED_IMAGE_SIMPLE}/>
);

/**
 * Renders an empty state with a loading spinner.
 *
 * @return
 */
const PathLoadingEmpty = () => (
    <Empty
        image={Empty.PRESENTED_IMAGE_SIMPLE}
        description={<Spin
            size={"default"}
            spinning={true}/>}/>
);

/**
 * Renders a compression job submission form.
 *
 * @return
 */
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [submitResult, setSubmitResult] = useState<{success: boolean; message: string} | null>(null);
    const [treeData, setTreeData] = useState<TreeNode[]>([{id: "/", value: "/", title: "/", isLeaf: false}]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);
    const [isLoading, setIsLoading] = useState(false);

    const fetchAndAppendTreeNodes = useCallback(async (path: string): Promise<boolean> => {
        try {
            const {data} = await listFiles(path);
            const newNodes = data.map(mapFileToTreeNode);

            setTreeData((prev) => {
                // Create a map of existing node IDs for quick lookup
                const existingNodeIds = new Set(prev.map((node) => node.id));

                // Filter out nodes that already exist
                const uniqueNewNodes = newNodes.filter((node) => !existingNodeIds.has(node.id));

                return [
                    ...prev,
                    ...uniqueNewNodes,
                ];
            });

            // automatically expand the parent node
            setExpandedKeys((prev) => Array.from(new Set([...prev,
                path])));

            return true;
        } catch (e) {
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while loading paths");

            return false;
        }
    }, []);

    /*
     * Load missing parent nodes for a given path.
     */
    const loadMissingParents = useCallback(async (path: string): Promise<boolean> => {
        const pathSegments = path.split("/").filter((segment) => 0 < segment.length);
        let currentPath = "/";

        // Load root if not present
        if (!treeData.some((node) => "/" === node.id)) {
            const success = await fetchAndAppendTreeNodes("/");
            if (!success) {
                return false;
            }
        }

        // Load each parent level
        for (let i = 0; i < pathSegments.length; i++) {
            const segment = pathSegments[i];
            const parentPath = currentPath;
            currentPath = "/" === currentPath ?
                `/${segment}` :
                `${currentPath}/${segment}`;

            // Check if node already exists
            if (!treeData.some((node) => node.id === currentPath)) {
                const success = await fetchAndAppendTreeNodes(parentPath);
                if (!success) {
                    return false;
                }
            }
        }

        return true;
    }, [treeData,
        fetchAndAppendTreeNodes]);

    const handleLoadData = useCallback<NonNullable<TreeSelectProps["loadData"]>>(async (node) => {
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
    }, [fetchAndAppendTreeNodes]);

    const handleSearch = useCallback<NonNullable<TreeSelectProps["onSearch"]>>(async (value) => {
        if (!value.trim()) {
            return;
        }

        setIsLoading(true);
        try {
            // Extract the base directory from the search string
            let basePath: string;
            if (value.endsWith("/")) {
                // If it ends with "/", treat it as a directory
                basePath = "/" === value ?
                    "/" :
                    value.slice(0, -1);
            } else {
                // If it's a file path, extract the directory part
                const lastSlashIndex = value.lastIndexOf("/");
                if (-1 === lastSlashIndex) {
                    // No slash found, assume root directory
                    basePath = "/";
                } else if (0 === lastSlashIndex) {
                    // Path starts with "/" but has no other slashes, use root
                    basePath = "/";
                } else {
                    // Extract directory path
                    basePath = value.substring(0, lastSlashIndex);
                }
            }

            // Check if the base directory is already expanded to avoid unnecessary API calls
            if (expandedKeys.includes(basePath)) {
                return;
            }

            const parentsLoaded = await loadMissingParents(basePath);
            if (parentsLoaded) {
                await fetchAndAppendTreeNodes(basePath);
            }
        } finally {
            setIsLoading(false);
        }
    }, [fetchAndAppendTreeNodes,
        loadMissingParents,
        expandedKeys]);

    const handleTreeExpand = useCallback<NonNullable<TreeSelectProps["onTreeExpand"]>>((keys) => {
        setExpandedKeys(keys);
    }, []);

    const handleSubmit = async (values: FormValues) => {
        setIsSubmitting(true);
        setSubmitResult(null);
        try {
            const jobId = await submitCompressionJob({
                paths: values.paths,
                dataset: values.dataset,
                timestampKey: values.timestampKey,
            });

            setSubmitResult({success: true, message: `Compression job submitted successfully with ID: ${jobId}`});
            form.resetFields();
        } catch (error) {
            setSubmitResult({
                success: false,
                message: `Failed to submit compression job: ${error instanceof Error ?
                    error.message :
                    "Unknown error"}`,
            });
        } finally {
            setIsSubmitting(false);
        }
    };

    const isClpS = "clp-s" === settings.ClpStorageEngine;

    return (
        <DashboardCard title={"Submit Compression Job"}>
            <Form
                form={form}
                layout={"vertical"}
                onFinish={handleSubmit}
            >
                <Form.Item
                    label={"Paths"}
                    name={"paths"}
                    rules={[{required: true, message: "Please select at least one path"}]}
                >
                    <TreeSelect
                        allowClear={true}
                        listHeight={512}
                        loadData={handleLoadData}
                        multiple={true}
                        placeholder={"Please select paths to compress"}
                        showCheckedStrategy={TreeSelect.SHOW_PARENT}
                        showSearch={true}
                        treeCheckable={true}
                        treeData={treeData}
                        treeDataSimpleMode={true}
                        treeExpandAction={"click"}
                        treeExpandedKeys={expandedKeys}
                        treeLine={true}
                        treeNodeLabelProp={"value"}
                        notFoundContent={isLoading ?
                            <PathLoadingEmpty/> :
                            <PathNotFoundEmpty/>}
                        switcherIcon={(props) => (props.expanded ?
                            <MinusOutlined style={{color: "grey"}}/> :
                            <PlusOutlined style={{color: "grey"}}/>)}
                        onSearch={handleSearch}
                        onTreeExpand={handleTreeExpand}/>
                </Form.Item>
                {isClpS && (
                    <>
                        <Form.Item
                            label={"Dataset"}
                            name={"dataset"}
                        >
                            <Input placeholder={"The dataset that the archives belong to (optional)"}/>
                        </Form.Item>
                        <Form.Item
                            label={"Timestamp Key"}
                            name={"timestampKey"}
                        >
                            <Input placeholder={"The path for the field containing the log event's timestamp (optional)"}/>
                        </Form.Item>
                    </>
                )}

                <Form.Item>
                    <Button
                        htmlType={"submit"}
                        loading={isSubmitting}
                        type={"primary"}
                    >
                        {isSubmitting ?
                            "Submitting..." :
                            "Submit"}
                    </Button>
                </Form.Item>

                {submitResult && (
                    <Typography.Text
                        type={submitResult.success ?
                            "success" :
                            "danger"}
                    >
                        {submitResult.message}
                    </Typography.Text>
                )}
            </Form>
        </DashboardCard>
    );
};

export default Compress;

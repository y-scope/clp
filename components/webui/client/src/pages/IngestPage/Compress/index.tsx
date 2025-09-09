import {
    useCallback,
    useState,
} from "react";

import {
    Button,
    Empty,
    Form,
    GetProp,
    Input,
    message,
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
 *
 */
const PathNotFoundEmpty = () => (
    <Empty
        description={"Path not found"}
        image={Empty.PRESENTED_IMAGE_SIMPLE}/>
);

/**
 *
 */
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [submitResult, setSubmitResult] = useState<{success: boolean; message: string} | null>(null);
    const [treeData, setTreeData] = useState<TreeNode[]>([{id: "/", value: "/", title: "/", isLeaf: false}]);
    const [expandedKeys, setExpandedKeys] = useState<string[]>([]);

    const fetchAndAppendTreeNodes = useCallback(async (path: string) => {
        try {
            const {data} = await listFiles(path);
            const newNodes = data.map(mapFileToTreeNode);

            setTreeData((prev) => prev.concat(newNodes));

            // automatically expand the parent node
            setExpandedKeys((prev) => Array.from(new Set([...prev,
                path])));
        } catch (e) {
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while loading paths");
        }
    }, []);

    const handleLoadData = useCallback<NonNullable<TreeSelectProps["loadData"]>>(async (node) => {
        const path = node.value;
        if ("string" !== typeof path) {
            return;
        }
        await fetchAndAppendTreeNodes(path);
    }, [fetchAndAppendTreeNodes]);

    const handleSearch = useCallback<NonNullable<TreeSelectProps["onSearch"]>>(async (value) => {
        if (!value.endsWith("/")) {
            return;
        }
        const path = "/" === value ?
            "/" :
            value.slice(0, -1);

        await fetchAndAppendTreeNodes(path);
    }, [fetchAndAppendTreeNodes]);

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
                        notFoundContent={<PathNotFoundEmpty/>}
                        placeholder={"Please select paths to compress"}
                        showCheckedStrategy={TreeSelect.SHOW_PARENT}
                        showSearch={true}
                        treeCheckable={true}
                        treeData={treeData}
                        treeDataSimpleMode={true}
                        treeExpandAction={"click"}
                        treeExpandedKeys={expandedKeys}
                        treeNodeLabelProp={"value"}
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

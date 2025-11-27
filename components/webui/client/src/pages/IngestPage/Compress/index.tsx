import {useState} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {
    Button,
    Empty,
    Form,
    Input,
    Spin,
    TreeSelect,
    Typography,
} from "antd";

import {submitCompressionJob} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {settings} from "../../../settings";
import useFileTree from "./useFileTree";


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
// eslint-disable-next-line max-lines-per-function
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [submitResult, setSubmitResult] = useState<{
        success: boolean;
        message: string;
    } | null>(null);

    const {
        treeData,
        expandedKeys,
        isLoading,
        handleLoadData,
        handleSearch,
        handleTreeExpand,
    } = useFileTree();

    const handleSubmit = async (values: FormValues) => {
        setIsSubmitting(true);
        setSubmitResult(null);
        try {
            const payload: {paths: string[]; dataset?: string; timestampKey?: string} = {
                paths: values.paths,
            };

            if (values.dataset) {
                payload.dataset = values.dataset;
            }
            if (values.timestampKey) {
                payload.timestampKey = values.timestampKey;
            }
            const result = await submitCompressionJob(payload);

            setSubmitResult({
                success: true,
                message: `Compression job submitted successfully with ID: ${result.jobId}`,
            });
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
                onFinish={(values) => {
                    // eslint-disable-next-line no-void
                    void handleSubmit(values);
                }}
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
                        switcherIcon={(props: {expanded?: boolean}) => (
                            props.expanded ?
                                <MinusOutlined style={{color: "grey"}}/> :
                                <PlusOutlined style={{color: "grey"}}/>
                        )}
                        onSearch={handleSearch}
                        onTreeExpand={handleTreeExpand}/>
                </Form.Item>
                {isClpS && (
                    <>
                        <Form.Item
                            label={"Dataset"}
                            name={"dataset"}
                        >
                            <Input
                                placeholder={"The dataset that the archives belong to (optional)"}/>
                        </Form.Item>
                        <Form.Item
                            label={"Timestamp Key"}
                            name={"timestampKey"}
                        >
                            <Input
                                placeholder={
                                    "The path for the field containing the log event's timestamp " +
                                    "(optional)"
                                }/>
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
